//My 4coder custom layer
//@todo add comment highlighting
//@todo custom theme
//@todo delete open file
//@todo improve prototypeify so it elimenates whitespace and comments
//@todo add project loaders (this will have to work with different paths on different computers
//@todo make default text mode always stay on same level of indentation
//@todo make text mode highlight matching parens
//@todo alt+del
//@simplify our replace stuff in buffers functions could share a lot more code

/* Bug in shift-tab
    if($ss.getUser().getRoles().contains("PMDCATS")){
      args.put("pmdcats","true");
    }
*/

#if !defined(FCODER_DEFAULT_BINDINGS_CPP)
#define FCODER_DEFAULT_BINDINGS_CPP

#define EXTRA_KEYWORDS "fox_keywords.hpp"

#include "4coder_default_include.cpp"

#define declare_stack_buffered_string(name, cap) \
char name##_buffer[cap]; \
auto name = make_string_cap(name##_buffer, 0, cap);

#define fox_for(iter_name, iter_count) for (uint32_t iter_name = 0; iter_name < (iter_count); ++iter_name)

CUSTOM_COMMAND_SIG(smart_tab)
CUSTOM_DOC("Intelligently inserts a tab or word completes")
{
	auto active_view = get_active_view(app, AccessProtected);
	auto active_buffer = get_buffer(app, active_view.buffer_id, AccessProtected);
	
	auto cursor_pos = active_view.cursor.pos;
	b32_4tech write_tab = true;
	if (cursor_pos) {
		char left_of_cursor;
		if (buffer_read_range(app, &active_buffer, cursor_pos - 1, cursor_pos, &left_of_cursor)) {
			write_tab = char_is_whitespace(left_of_cursor);
		}
	}
	
	if (write_tab) {
		write_string(app, lit("\t"));
	} else {
		word_complete(app);
	}
}

//Identation
void change_indentation_level(Application_Links* app, bool indent_more) {
	auto active_view = get_active_view(app, AccessProtected);
	auto active_buffer = get_buffer(app, active_view.buffer_id, AccessProtected);
	
	auto cursor_pos = active_view.cursor.pos;
	auto mark_pos = active_view.mark.pos;
	uint32_t start_pos;
	uint32_t end_pos;
	if (cursor_pos != mark_pos) {
		if (cursor_pos < mark_pos) {
			start_pos = cursor_pos;
			end_pos = mark_pos;
		} else {
			start_pos = mark_pos;
			end_pos = cursor_pos;
		}
		
		//Put it one past because thats what we need
		end_pos++;
		
		auto contents_size = end_pos - start_pos;
		constexpr uint32_t buffer_size = 32768;
		constexpr uint32_t max_line_count = 300;
		if (contents_size < buffer_size) {
			auto contents = (char*)malloc(buffer_size);
			if (buffer_read_range(app, &active_buffer, start_pos, end_pos, contents)) {
				uint32_t cur = 0;
				while (cur < contents_size) {
					if (indent_more) {
						auto move_dest = contents + cur + 1;
						auto move_src = contents + cur;
						auto move_size = contents_size - cur;
						if (move_dest + move_size > contents + buffer_size) {
							return; //Dont move out of bounds
						}
						memmove(move_dest, move_src, move_size);
						
						contents[cur] = '\t';
						contents_size++;
					} else {
						if (contents[cur] == '\t') {
							auto move_dest = contents + cur;
							auto move_src = contents + cur + 1;
							auto move_size = contents_size - cur;
							memmove(move_dest, move_src, move_size);
							
							contents_size--;
						} else if (contents[cur] == ' ') {
							uint32_t iend_spaces;
							for (iend_spaces = cur; iend_spaces < contents_size; iend_spaces++) {
								if (contents[iend_spaces] != ' ') {
									break;
								}
							}
							
							auto move_dest = contents + cur;
							auto move_src = contents + iend_spaces;
							auto move_size = contents_size - cur;
							memmove(move_dest, move_src, move_size);
							
							contents_size -= (uint32_t)(move_src - move_dest);
						}
					}
					
					//scan to next line ending we will either advance cur until cur == contents_size and thus break the outer loop or find the next line ending
					while (true) {
						cur++;
						if (cur < contents_size) {
							if (contents[cur] == '\n') {
								cur++;
								if (cur < contents_size) {
									if (contents[cur] == '\n' || contents[cur] == '\r') {
										//This is another line ending so we don't actually want to insert or subtract space
										//And we don't want to skip processing it
										cur--;
									} else {
										break;
									}
								} else {
									break;
								}
							}
						} else {
							break;
						}
					}
				}
				
				buffer_replace_range(app, &active_buffer, start_pos, end_pos, contents, contents_size);
				
				//Keep the mark and cursor in the same spot so we can spam this command
				Buffer_Seek mark_seek = {};
				mark_seek.pos = start_pos;
				Buffer_Seek cursor_seek = {};
				cursor_seek.pos = start_pos + contents_size - 1;
				
				view_set_mark(app, &active_view, mark_seek);
				view_set_cursor(app, &active_view, cursor_seek, true);
			}
			
			free(contents);
		}
	}
}

//Tabs to Spaces
CUSTOM_COMMAND_SIG(tabs_to_spaces)
CUSTOM_DOC("Turns tabs into 4 spaces.")
{
	auto active_view = get_active_view(app, AccessProtected);
	auto active_buffer = get_buffer(app, active_view.buffer_id, AccessProtected);
	
	uint32_t contents_size = (uint32_t)active_buffer.size;
	auto old_contents_size = contents_size;
	constexpr uint32_t buffer_size = 65536;
	constexpr uint32_t max_line_count = 300;
	if (contents_size < buffer_size) {
		auto contents = (char*)malloc(buffer_size);
		if (buffer_read_range(app, &active_buffer, 0, contents_size, contents)) {
			fox_for (i, old_contents_size) {
				if (contents[i] == '\t') {
					auto dest = contents + i + 4; //Room for 4 spaces
					auto src = contents + i + 1; //Content after tab
					auto size = contents + contents_size - src; //Copy till the end of the buffer
					memmove(dest, src, size);
					
					//Fill in spaces
					fox_for (ispace, 4) {
						contents[i + ispace] = ' ';
					}
					
					i += 3; //Skip the spaces we just added, leave one since i will increment anyway
					contents_size += 3; //Add the additional size to the buffer
				}
			}
			
			buffer_replace_range(app, &active_buffer, 0, old_contents_size, contents, contents_size);
		}
		free(contents);
	}
}

CUSTOM_COMMAND_SIG(increase_indent)
CUSTOM_DOC("Tabs over text by one tab.")
{
	change_indentation_level(app, true);
}

CUSTOM_COMMAND_SIG(decrease_indent)
CUSTOM_DOC("Tabs text backward one tab.")
{
	change_indentation_level(app, false);
}

//Character insertion shortcuts
CUSTOM_COMMAND_SIG(marker_comment)
CUSTOM_DOC("Inserts //---.")
{
	write_string(app, lit("//---"));
}

//Prototypeify
CUSTOM_COMMAND_SIG(prototypeify)
CUSTOM_DOC("Turns a block of function definitions into a block of prototypes.")
{
	auto active_view = get_active_view(app, AccessProtected);
	auto active_buffer = get_buffer(app, active_view.buffer_id, AccessProtected);
	
	auto cursor_pos = active_view.cursor.pos;
	auto mark_pos = active_view.mark.pos;
	uint32_t start_pos;
	uint32_t end_pos;
	if (cursor_pos != mark_pos) {
		if (cursor_pos < mark_pos) {
			start_pos = cursor_pos;
			end_pos = mark_pos;
		} else {
			start_pos = mark_pos;
			end_pos = cursor_pos;
		}
		
		//Put it one past because thats what we need
		end_pos++;
		
		auto contents_size = end_pos - start_pos;
		constexpr uint32_t buffer_size = 32768;
		constexpr uint32_t max_line_count = 300;
		if (contents_size < buffer_size) {
			auto contents = (char*)malloc(buffer_size);
			if (buffer_read_range(app, &active_buffer, start_pos, end_pos, contents)) {
				for (uint32_t ichar = 0;
					 ichar < contents_size;
					 ++ichar) {
					//Find function body
					if (contents[ichar] == '{') {
						uint32_t brace_count = 1;
						uint32_t ibody = ichar;
						
						ichar++;
						while (true) {
							if (contents[ichar] == '}') {
								brace_count--;
								if (!brace_count) {
									break;
								}
							} else if (contents[ichar] == '{') {
								brace_count++;
							} else if (ichar >= contents_size) {
								goto double_break;
							}
							ichar++;
						}
						
						ichar++;
						contents[ibody - 1] = ';';
						auto move_dest = contents + ibody;
						auto move_src = contents + ichar;
						auto move_size = contents_size - ichar;
						memmove(move_dest, move_src, move_size);
						
						contents_size -= ichar - ibody;
						ichar = ibody;
					}
				}
				
				double_break:
				buffer_replace_range(app, &active_buffer, start_pos, end_pos, contents, contents_size);
			}
			free(contents);
		}
	}
}

//Scroll
static void scroll_offset(Application_Links* app, int32_t scroll_offset) {
	auto active_view = get_active_view(app, AccessProtected);
	auto new_scroll = active_view.scroll_vars;
	new_scroll.target_y -= scroll_offset;
	view_set_scroll(app,
					&active_view,
					new_scroll);
}

constexpr int32_t scroll_amount = 170;

CUSTOM_COMMAND_SIG(scroll_up)
CUSTOM_DOC("Scrolls several lines up.")
{
	scroll_offset(app, scroll_amount);
}

CUSTOM_COMMAND_SIG(scroll_down)
CUSTOM_DOC("Scrolls several lines down.")
{
	scroll_offset(app, -scroll_amount);
}

//Setup various commands operating on alphanumeric/underscore words
CUSTOM_COMMAND_SIG(seek_alphanumeric_or_underscore_left)
CUSTOM_DOC("Seek to the left the next alphanumeric token including underscores.")
{
	auto active_view = get_active_view(app, AccessProtected);
	auto active_buffer = get_buffer(app, active_view.buffer_id, AccessProtected);
	auto new_cursor = active_view.cursor;
	auto new_pos = buffer_seek_alphanumeric_or_underscore_left(
		app,
		&active_buffer,
		new_cursor.pos
		);
	auto sought_pos = seek_pos(new_pos);
	view_set_cursor(app, &active_view, sought_pos, true);
}

CUSTOM_COMMAND_SIG(seek_alphanumeric_or_underscore_right)
CUSTOM_DOC("Seek to the right the next alphanumeric token including underscores.")
{
	auto active_view = get_active_view(app, AccessProtected);
	auto active_buffer = get_buffer(app, active_view.buffer_id, AccessProtected);
	auto new_cursor = active_view.cursor;
	auto new_pos = buffer_seek_alphanumeric_or_underscore_right(
		app,
		&active_buffer,
		new_cursor.pos
		);
	auto sought_pos = seek_pos(new_pos);
	view_set_cursor(app, &active_view, sought_pos, true);
}

CUSTOM_COMMAND_SIG(cut_word)
CUSTOM_DOC("Cut the word under the cursor.")
{
	seek_alphanumeric_or_underscore_left(app);
	set_mark(app);
	seek_alphanumeric_or_underscore_right(app);
	cut(app);
}

CUSTOM_COMMAND_SIG(copy_word)
CUSTOM_DOC("Copy the word under the cursor.")
{
	seek_alphanumeric_or_underscore_left(app);
	set_mark(app);
	seek_alphanumeric_or_underscore_right(app);
	copy(app);
}

CUSTOM_COMMAND_SIG(replace_word)
CUSTOM_DOC("Replaces the word under the cursor.")
{
	seek_alphanumeric_or_underscore_left(app);
	set_mark(app);
	seek_alphanumeric_or_underscore_right(app);
	replace_range_with_paste(app);
}

CUSTOM_COMMAND_SIG(backspace_underscore_word)
CUSTOM_DOC("Delete the underscore word to the left.")
{
	set_mark(app);
	seek_alphanumeric_or_underscore_left(app);
	cut(app);
}

CUSTOM_COMMAND_SIG(delete_underscore_word)
CUSTOM_DOC("Delete the underscore word to the right.")
{
	set_mark(app);
	seek_alphanumeric_or_underscore_right(app);
	cut(app);
}

//Ctrl+home/end with marking
CUSTOM_COMMAND_SIG(mark_and_goto_beginning_of_file)
CUSTOM_DOC("Sets the mark at the cursor and goes to the beginning of the file.")
{
	set_mark(app);
	goto_beginning_of_file(app);
}

CUSTOM_COMMAND_SIG(mark_and_goto_end_of_file)
CUSTOM_DOC("Sets the mark at the cursor and goes to the end of the file.")
{
	set_mark(app);
	goto_end_of_file(app);
}

//Select whole file then copy contents
CUSTOM_COMMAND_SIG(select_file_and_copy)
CUSTOM_DOC("Selects the entire file and copies it")
{
	goto_end_of_file(app);
	set_mark(app);
	goto_beginning_of_file(app);
	copy(app);
}

//Cut/replace stuff
CUSTOM_COMMAND_SIG(replace_range_with_paste)
CUSTOM_DOC("Replaces the currently selected paste with whatever is on the clipboard.")
{
	delete_range(app);
	paste(app);
}

static void select_line(Application_Links* app) {
	seek_beginning_of_textual_line(app);
	set_mark(app);
	seek_end_of_textual_line(app);
	move_right(app); //Put us on the next line so we can spam this
}

CUSTOM_COMMAND_SIG(copy_line)
CUSTOM_DOC("Copy the current line.")
{
	select_line(app);
	copy(app);
}

CUSTOM_COMMAND_SIG(cut_line)
CUSTOM_DOC("Cut the current line.")
{
	select_line(app);
	cut(app);
}

CUSTOM_COMMAND_SIG(replace_line)
CUSTOM_DOC("Replace the current line with the clipboard.")
{
	select_line(app);
	delete_range(app);
	paste(app);
}

//Kill whitespace
CUSTOM_COMMAND_SIG(kill_whitespace)
CUSTOM_DOC("Removes the whitespace at the cursor.")
{
	seek_whitespace_left(app);
	seek_whitespace_right(app);
	set_mark(app);
	seek_whitespace_right(app);
	seek_whitespace_left(app);
	delete_range(app);
}

//Open in explorer
CUSTOM_COMMAND_SIG(explorer)
CUSTOM_DOC("Open the directory of the current file in windows explorer.")
{
	auto active_view = get_active_view(app, AccessProtected);
	auto active_buffer = get_buffer(app, active_view.buffer_id, AccessProtected);
	
	auto file_name = active_buffer.file_name;
	auto file_name_len = active_buffer.file_name_len;
	auto index_of_slash = file_name_len - 1;
	while (file_name[index_of_slash] != '\\') {
		index_of_slash--;
		if (!index_of_slash) {
			String error_msg = lit("Explorer command error: failed to find slash in file name. File name was ");
			print_message(app, error_msg.str, error_msg.size);
			print_message(app, file_name, file_name_len);
		}
	}
	
	declare_stack_buffered_string(command, 1024);
	append_checked_ss(&command, lit("explorer "));
	append_checked_ss(&command, make_string(file_name, index_of_slash));
	Buffer_Identifier output_buffer = {};
	exec_system_command(app,
						nullptr,
						output_buffer,
						file_name,
						index_of_slash,
						command.str,
						command.size,
						0);
}

//Project commands
CUSTOM_COMMAND_SIG(load_project_and_code)
CUSTOM_DOC("Opens all code in the current directory and loads the project.")
{
	load_project(app);
	open_all_code(app);
}

//Kill all open buffers
CUSTOM_COMMAND_SIG(kill_all_buffers)
CUSTOM_DOC("Kill all open buffers.")
{
	//First get all buffers, we cant kill them while looping through
	auto buffer_count_signed = get_buffer_count(app);
	if (buffer_count_signed > 0) {
		auto buffer_count = (uint32_t)buffer_count_signed;
		Buffer_Summary* all_buffers = (Buffer_Summary*)calloc(buffer_count,
															  sizeof(Buffer_Summary));
		
		{
			auto cur_buffer = get_buffer_first(app, AccessOpen);
			uint32_t ibuffer = 0;
			while (cur_buffer.exists) {
				all_buffers[ibuffer++] = cur_buffer;
				get_buffer_next(app, &cur_buffer, AccessOpen);
			}
		}
		
		//Now kill them all >:3
		fox_for (ibuffer, buffer_count) {
			auto buffer_name = make_string(all_buffers[ibuffer].buffer_name,
										   all_buffers[ibuffer].buffer_name_len);
			//For some reason this was killing three no-name buffers so im just gonna not do that
			if (buffer_name.size) {
				Buffer_Identifier buffer_id = {};
				buffer_id.id = all_buffers[ibuffer].buffer_id;
				
				auto kill_result = kill_buffer(app,
											   buffer_id,
											   (Buffer_Kill_Flag)0);
				//Error checking	
				switch (kill_result) {
					case BufferKillResult_Killed: {
						//Do nothing
					} break;
					
					case BufferKillResult_Dirty: {
						//Tried to kill a dirty buffer, tell the user and dont kill
						declare_stack_buffered_string(unsaved_msg, 512);
						append_checked_ss(&unsaved_msg, lit("Couldn't close buffer "));
						append_checked_ss(&unsaved_msg, buffer_name);
						append_checked_ss(&unsaved_msg, lit(" because it was unsaved."));
						print_message(app, unsaved_msg.str, unsaved_msg.size);
					} break;
					
					case BufferKillResult_Unkillable: {
						//Do nothing, we dont want to kill these.
					} break;
					
					case BufferKillResult_DoesNotExist: {
						declare_stack_buffered_string(err_msg, 512);
						append_checked_ss(&err_msg, lit("Error: Tried to kill the buffer "));
						append_checked_ss(&err_msg, buffer_name);
						append_checked_ss(&err_msg, lit(" but it didn't exist!"));
						print_message(app, err_msg.str, err_msg.size);
					} break;
				}
			}
		}
		
		free(all_buffers);
	}	
}

//Alignment functions
//This giant thing could use some error reporting
CUSTOM_COMMAND_SIG(align_comments)
CUSTOM_DOC("Aligns C line comments.")
{
	auto active_view = get_active_view(app, AccessProtected);
	auto active_buffer = get_buffer(app, active_view.buffer_id, AccessProtected);
	
	auto cursor_pos = active_view.cursor.pos;
	auto mark_pos = active_view.mark.pos;
	uint32_t start_pos;
	uint32_t end_pos;
	if (cursor_pos != mark_pos) {
		if (cursor_pos < mark_pos) {
			start_pos = cursor_pos;
			end_pos = mark_pos;
		} else {
			start_pos = mark_pos;
			end_pos = cursor_pos;
		}
		
		//Put it one past because thats what we need
		end_pos++;
		
		auto contents_size = end_pos - start_pos;
		constexpr uint32_t buffer_size = 4096;
		constexpr uint32_t max_line_count = 80;
		if (contents_size < buffer_size) {
			char contents[buffer_size];
			if (buffer_read_range(app, &active_buffer, start_pos, end_pos, contents)) {
				struct LineInfo {
					bool32 no_comment;
					uint32_t line_start;
					uint32_t code_end; //one past the end of code before the comment and whitespace
					uint32_t comment_start;
				};
				LineInfo line_infos[max_line_count];
				
				//Get line info
				uint32_t line_count = 0;
				int32_t largest_code_end = 0;
				{
					int32_t iline = 0;
					int32_t ichar = 0;
					int32_t line_start = 0;
					while (ichar < (int32_t)contents_size) {
						int32_t comment_start = 0;
						int32_t code_end = 0;
						bool32 no_comment = false;
						
						while (true) {
							if (ichar + 1 >= (int32_t)contents_size) {
								no_comment = true;
								break;
							}
							
							//Find the comment
							if (contents[ichar] == '/'
								&& contents[ichar + 1] == '/') {
								comment_start = ichar - line_start;
								break;
							}
							
							if (contents[ichar] == '\n') {
								no_comment = true;
								break;
							}
							
							ichar++;
						}
						
						int32_t next_line_start = 0;
						if (no_comment) {
							next_line_start = ichar + 1;
						} else {
							//Trace backwards to find code_end
							for (ichar -= 1;
								 ichar >= line_start;
								 --ichar) {
								if (!char_is_whitespace(contents[ichar])) {
									code_end = ichar + 1 - line_start;
									break;
								}
							}
							
							//If we dont break early end_code = 0 which is what we want
							//When a line is only comment and whitespace
							
							if (code_end > largest_code_end) {
								largest_code_end = code_end;
							}
							
							//Trace forwards to find end of line
							for (ichar = line_start + comment_start;
								 ichar < (int32_t)contents_size;
								 ichar++) {
								if (contents[ichar] == '\n') {
									break;
								}
							}
							next_line_start = ichar + 1;
						}
						
						line_infos[iline].no_comment = no_comment;
						line_infos[iline].line_start = (uint32_t)line_start;
						line_infos[iline].code_end = (uint32_t)code_end;
						line_infos[iline].comment_start = (uint32_t)comment_start;
						
						iline++;
						line_count = (uint32_t)iline;
						ichar = line_start = next_line_start;
					}
				}
				
				//New index relative to the start of the line where all comments will be
				uint32_t new_comment_start_offset = (uint32_t)largest_code_end + 1;
				//We are writing into contents so we need to offset all the old indices
				uint32_t contents_offset = 0;
				fox_for (iline, line_count) {
					auto line_info = line_infos + iline;
					
					if (line_info->no_comment) {
						continue;
					}
					
					//Move the comment and all text past it to the right place
					auto old_comment_start = line_info->line_start + line_info->comment_start + contents_offset;
					auto new_comment_start = line_info->line_start + new_comment_start_offset + contents_offset;
					auto move_dest = contents + new_comment_start;
					auto move_src = contents + old_comment_start;
					auto move_size = (contents_size + contents_offset) - line_info->comment_start;
					if (move_dest + move_size > contents + buffer_size) {
						return; //Dont move out of bounds
					}
					memmove(move_dest, move_src, move_size);
					
					//Fill the space with spaces
					fox_for (i, new_comment_start_offset - line_info->code_end) {
						auto ispace = i + line_info->code_end + line_info->line_start;
						contents[ispace + contents_offset] = ' ';
					}
					
					contents_offset += new_comment_start - old_comment_start;
				}
				
				//Write back final results
				buffer_replace_range(app, &active_buffer, start_pos, end_pos, contents, contents_size + contents_offset);
			}
		}
	}
}

CUSTOM_COMMAND_SIG(close_other_panel)
CUSTOM_DOC("Closes the panel...no not that one, the other one.")
{
	change_active_panel(app);
	close_panel(app);
}

void fox_keys(Bind_Helper *context) {
	begin_map(context, mapid_global);
	bind(context, '`', MDFR_CTRL, change_active_panel);
	bind(context, '<', MDFR_CTRL, change_active_panel_backwards);
	bind(context, 'n', MDFR_CTRL, interactive_new);
	bind(context, 'o', MDFR_CTRL, interactive_open_or_new);
	bind(context, 'o', MDFR_ALT, open_in_other);
	bind(context, 'p', MDFR_CTRL, load_project_and_code);
	bind(context, 'l', MDFR_CTRL, kill_all_buffers);
	bind(context, 'm', MDFR_CTRL, toggle_virtual_whitespace);
	bind(context, 'k', MDFR_CTRL, kill_buffer);
	bind(context, '~', MDFR_CTRL, interactive_switch_buffer);
	bind(context, 'h', MDFR_CTRL, project_go_to_root_directory);
	bind(context, 'S', MDFR_CTRL, save_all_dirty_buffers);
	bind(context, '.', MDFR_ALT, change_to_build_panel);
	bind(context, ',', MDFR_ALT, close_build_panel);
	bind(context, 'n', MDFR_ALT, goto_next_jump_no_skips_sticky);
	bind(context, 'N', MDFR_ALT, goto_prev_jump_no_skips_sticky);
	bind(context, 'M', MDFR_ALT, goto_first_jump_sticky);
	bind(context, 'b', MDFR_ALT, toggle_filebar);
	bind(context, 'z', MDFR_ALT, execute_any_cli);
	bind(context, 'Z', MDFR_ALT, execute_previous_cli);
	bind(context, 'x', MDFR_ALT, command_lister);
	bind(context, 'X', MDFR_ALT, project_command_lister);
	bind(context, 'I', MDFR_CTRL, list_all_functions_current_buffer_lister);
	bind(context, 'E', MDFR_ALT, exit_4coder);
	bind(context, key_f1, MDFR_NONE, project_fkey_command);
	bind(context, key_f2, MDFR_NONE, project_fkey_command);
	bind(context, key_f3, MDFR_NONE, project_fkey_command);
	bind(context, key_f4, MDFR_NONE, project_fkey_command);
	bind(context, key_f4, MDFR_ALT, exit_4coder);
	bind(context, key_f5, MDFR_NONE, project_fkey_command);
	bind(context, key_f6, MDFR_NONE, project_fkey_command);
	bind(context, key_f7, MDFR_NONE, project_fkey_command);
	bind(context, key_f8, MDFR_NONE, project_fkey_command);
	bind(context, key_f9, MDFR_NONE, project_fkey_command);
	bind(context, key_f10, MDFR_NONE, project_fkey_command);
	bind(context, key_f11, MDFR_NONE, project_fkey_command);
	bind(context, key_f12, MDFR_NONE, project_fkey_command);
	bind(context, key_f13, MDFR_NONE, project_fkey_command);
	bind(context, key_f14, MDFR_NONE, project_fkey_command);
	bind(context, key_f15, MDFR_NONE, project_fkey_command);
	bind(context, key_f16, MDFR_NONE, project_fkey_command);
	bind(context, key_mouse_wheel, MDFR_NONE, mouse_wheel_scroll);
	end_map(context);
	begin_map(context, mapid_file);
	bind_vanilla_keys(context, write_character);
	bind(context, '=', MDFR_ALT, increase_line_wrap);
	bind(context, '-', MDFR_ALT, decrease_line_wrap);
	bind(context, '-', MDFR_CTRL, marker_comment);
	bind(context, '\\', MDFR_CTRL, kill_whitespace);
	bind(context, key_mouse_left, MDFR_NONE, click_set_cursor_and_mark);
	bind(context, key_mouse_left_release, MDFR_NONE, click_set_cursor);
	bind(context, key_mouse_move, MDFR_NONE, click_set_cursor_if_lbutton);
	bind(context, key_del, MDFR_NONE, delete_char);
	bind(context, key_del, MDFR_SHIFT, delete_char);
	bind(context, key_back, MDFR_NONE, backspace_char);
	bind(context, key_back, MDFR_SHIFT, backspace_char);
	bind(context, key_up, MDFR_NONE, move_up);
	bind(context, key_down, MDFR_NONE, move_down);
	bind(context, key_left, MDFR_NONE, move_left);
	bind(context, key_right, MDFR_NONE, move_right);
	bind(context, key_up, MDFR_SHIFT, move_up);
	bind(context, key_down, MDFR_SHIFT, move_down);
	bind(context, key_left, MDFR_SHIFT, move_left);
	bind(context, key_right, MDFR_SHIFT, move_right);
	bind(context, key_end, MDFR_NONE, seek_end_of_line);
	bind(context, key_home, MDFR_NONE, seek_beginning_of_line);
	bind(context, key_end, MDFR_SHIFT, seek_end_of_line);
	bind(context, key_home, MDFR_SHIFT, seek_beginning_of_line);
	bind(context, key_end, MDFR_CTRL, mark_and_goto_end_of_file);
	bind(context, key_home, MDFR_CTRL, mark_and_goto_beginning_of_file);
	bind(context, key_page_up, MDFR_CTRL|MDFR_SHIFT, goto_beginning_of_file);
	bind(context, key_page_down, MDFR_CTRL|MDFR_SHIFT, goto_end_of_file);
	bind(context, key_page_up, MDFR_SHIFT, page_up);
	bind(context, key_page_down, MDFR_SHIFT, page_down);
	bind(context, key_up, MDFR_CTRL, seek_whitespace_up_end_line);
	bind(context, key_down, MDFR_CTRL, seek_whitespace_down_end_line);
	bind(context, key_right, MDFR_CTRL, seek_alphanumeric_or_underscore_right);
	bind(context, key_left, MDFR_CTRL, seek_alphanumeric_or_underscore_left);
	bind(context, key_up, MDFR_CTRL|MDFR_SHIFT, seek_whitespace_up_end_line);
	bind(context, key_down, MDFR_CTRL|MDFR_SHIFT, seek_whitespace_down_end_line);
	bind(context, key_right, MDFR_CTRL|MDFR_SHIFT, seek_alphanumeric_or_underscore_right);
	bind(context, key_left, MDFR_CTRL|MDFR_SHIFT, seek_alphanumeric_or_underscore_left);
	bind(context, key_up, MDFR_ALT, scroll_up);
	bind(context, key_down, MDFR_ALT, scroll_down);
	bind(context, key_back, MDFR_CTRL, backspace_underscore_word);
	bind(context, key_del, MDFR_CTRL, delete_underscore_word);
	bind(context, key_back, MDFR_ALT, snipe_token_or_word);
	bind(context, key_del, MDFR_ALT, snipe_token_or_word_right);
	bind(context, 'a', MDFR_CTRL, select_file_and_copy);
	bind(context, 'd', MDFR_CTRL, set_mark);
	bind(context, 'c', MDFR_CTRL, copy);
	bind(context, key_back, MDFR_ALT, copy_line);
	bind(context, key_back, MDFR_CTRL|MDFR_SHIFT, cut_line);
	bind(context, key_back, MDFR_CTRL|MDFR_ALT, replace_line);
	bind(context, 'f', MDFR_CTRL, search);
	bind(context, 'F', MDFR_CTRL, list_all_locations);
	bind(context, 'F', MDFR_ALT, list_all_substring_locations_case_insensitive);
	bind(context, 'f', MDFR_CTRL|MDFR_ALT, list_all_locations_of_identifier);
	bind(context, 'g', MDFR_CTRL, goto_line);
	bind(context, 'G', MDFR_CTRL, list_all_locations_of_selection);
	bind(context, 'j', MDFR_CTRL, snippet_lister);
	bind(context, 'K', MDFR_CTRL, kill_buffer);
	bind(context, 'L', MDFR_CTRL, to_lowercase);
	bind(context, 'd', MDFR_ALT, cursor_mark_swap);
	bind(context, 'O', MDFR_CTRL, reopen);
	bind(context, 'q', MDFR_CTRL, query_replace);
	bind(context, 'Q', MDFR_CTRL, query_replace_identifier);
	bind(context, 'q', MDFR_ALT, query_replace_selection);
	bind(context, 'r', MDFR_CTRL, reverse_search);
	bind(context, 's', MDFR_CTRL, save);
	bind(context, 't', MDFR_CTRL, search_identifier);
	bind(context, 'T', MDFR_CTRL, list_all_locations_of_identifier);
	bind(context, 'U', MDFR_CTRL, to_uppercase);
	bind(context, 'v', MDFR_CTRL, paste_and_indent);
	bind(context, 'V', MDFR_CTRL, paste_next_and_indent);
	bind(context, 'v', MDFR_CTRL|MDFR_ALT, replace_range_with_paste);
	bind(context, 'x', MDFR_CTRL, cut);
	bind(context, 'y', MDFR_CTRL, redo);
	bind(context, 'z', MDFR_CTRL, undo);
	bind(context, '1', MDFR_CTRL, close_other_panel);
	bind(context, '2', MDFR_CTRL, open_panel_vsplit);
	bind(context, '\n', MDFR_NONE, newline_or_goto_position_sticky);
	bind(context, '\n', MDFR_SHIFT, newline_or_goto_position_same_panel_sticky);
	bind(context, '\t', MDFR_CTRL, increase_indent);
	bind(context, '\t', MDFR_SHIFT, decrease_indent);
	bind(context, '\t', MDFR_NONE, smart_tab);
	bind(context, '>', MDFR_CTRL, view_jump_list_with_lister);
	bind(context, ' ', MDFR_SHIFT, write_character);
	bind(context, ' ', MDFR_CTRL, cut_word);
	bind(context, ' ', MDFR_ALT, copy_word);
	bind(context, ' ', MDFR_ALT|MDFR_CTRL, replace_word);
	end_map(context);
	begin_map(context, default_code_map);
	inherit_map(context, mapid_file);
	bind(context, '\n', MDFR_NONE, write_and_auto_tab);
	bind(context, '\n', MDFR_SHIFT, write_and_auto_tab);
	bind(context, '}', MDFR_NONE, write_and_auto_tab);
	bind(context, ')', MDFR_NONE, write_and_auto_tab);
	bind(context, ']', MDFR_NONE, write_and_auto_tab);
	bind(context, ';', MDFR_NONE, write_and_auto_tab);
	bind(context, '#', MDFR_NONE, write_and_auto_tab);
	bind(context, '\t', MDFR_NONE, word_complete);
	bind(context, 'h', MDFR_ALT, write_hack);
	bind(context, 'r', MDFR_ALT, write_block);
	bind(context, 't', MDFR_ALT, write_todo);
	bind(context, 'y', MDFR_ALT, write_note);
	bind(context, 'D', MDFR_ALT, list_all_locations_of_type_definition);
	bind(context, 'T', MDFR_ALT, list_all_locations_of_type_definition_of_identifier);
	bind(context, '[', MDFR_CTRL, open_long_braces);
	bind(context, '{', MDFR_CTRL, open_long_braces_semicolon);
	bind(context, '}', MDFR_CTRL, open_long_braces_break);
	bind(context, '[', MDFR_ALT, select_surrounding_scope);
	bind(context, ']', MDFR_ALT, select_prev_scope_absolute);
	bind(context, '\'', MDFR_ALT, select_next_scope_absolute);
	bind(context, '/', MDFR_ALT, place_in_scope);
	bind(context, 'j', MDFR_ALT, scope_absorb_down);
	bind(context, 'i', MDFR_ALT, if0_off);
	bind(context, '1', MDFR_ALT, open_file_in_quotes);
	bind(context, '2', MDFR_ALT, open_matching_file_cpp);
	bind(context, '0', MDFR_CTRL, write_zero_struct);
	end_map(context);
	begin_map(context, default_lister_ui_map);
	bind_vanilla_keys(context, lister__write_character);
	bind(context, key_esc, MDFR_NONE, lister__quit);
	bind(context, '\n', MDFR_NONE, lister__activate);
	bind(context, '\t', MDFR_NONE, lister__activate);
	bind(context, key_back, MDFR_NONE, lister__backspace_text_field);
	bind(context, key_up, MDFR_NONE, lister__move_up);
	bind(context, key_page_up, MDFR_NONE, lister__move_up);
	bind(context, key_down, MDFR_NONE, lister__move_down);
	bind(context, key_page_down, MDFR_NONE, lister__move_down);
	bind(context, key_mouse_wheel, MDFR_NONE, lister__wheel_scroll);
	bind(context, key_mouse_left, MDFR_NONE, lister__mouse_press);
	bind(context, key_mouse_left_release, MDFR_NONE, lister__mouse_release);
	bind(context, key_mouse_move, MDFR_NONE, lister__repaint);
	bind(context, key_animate, MDFR_NONE, lister__repaint);
	end_map(context);
}

extern "C" int32_t
get_bindings(void *data, int32_t size){
	Bind_Helper context_ = begin_bind_helper(data, size);
	Bind_Helper *context = &context_;
	
	set_all_default_hooks(context);
	fox_keys(context);
	
	int32_t result = end_bind_helper(context);
	return(result);
}

#endif //FCODER_DEFAULT_BINDINGS
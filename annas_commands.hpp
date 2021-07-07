//@todo prototypeify

//---Character insertion
CUSTOM_COMMAND_SIG(marker_comment)
CUSTOM_DOC("Inserts //---.")
{
	write_string(app, string_u8_litexpr("//---"));
    F4_PowerMode_CharacterPressed();
    F4_PowerMode_Spawn(app, get_active_view(app, Access_ReadWriteVisible), 0);
}

//---Copy/Paste/Selcetion commands
static void seek_alpha_numeric_underscore(Application_Links* app, Scan_Direction direction) {
    auto active_view_id = get_active_view(app, Access_ReadVisible);
    auto active_buffer_id = view_get_buffer(app, active_view_id, Access_ReadVisible);
    auto cursor_position = view_get_cursor_pos(app, active_view_id);
    
    auto seek_position = direction == Scan_Forward ? cursor_position : (cursor_position - 1);
    
    //There are two cases to consider depending on the character class at the cursor
    // 1. h e <cursor> l l o , should evaluate to <cursor> h e l l o , (left)
    //                                            h e l l o <cursor> , (right)
    // 2. h e l l o , <cursor> , h i should evaluate to <cursor> h e l l o , , h i (left)
    //                                                  h e l l o , , <cursor> h i (right)
    
    auto character_at_cursor = buffer_get_char(app, active_buffer_id, seek_position);
    if (!character_predicate_check_character(character_predicate_alpha_numeric_underscore_utf8, character_at_cursor)) {
        //Seek our desired character class
        auto anu_match = buffer_seek_character_class(app, active_buffer_id, &character_predicate_alpha_numeric_underscore_utf8, direction, seek_position);
        if (anu_match.range.min == anu_match.range.max) {
            return;
        }
        seek_position = anu_match.range.min;
    }
    
    //At alphanumeric, seek the next boundary of our desired character class
    auto not_alpha_numeric_underscore_utf8 = character_predicate_not(&character_predicate_alpha_numeric_underscore_utf8);
    auto non_anu_match = buffer_seek_character_class(app, active_buffer_id, &not_alpha_numeric_underscore_utf8, direction, seek_position);
    if (non_anu_match.range.min == non_anu_match.range.max) {
        return;
    }
    
    seek_position = non_anu_match.range.min + (direction == Scan_Forward ? 0 : 1);
    
    Buffer_Seek seek_match = {};
    seek_match.type = buffer_seek_pos;
    seek_match.pos = seek_position; 
    view_set_cursor(app, active_view_id, seek_match);
}

CUSTOM_COMMAND_SIG(seek_alpha_numeric_underscore_left)
CUSTOM_DOC("Seek the left boundary of the nearest alpha-numeric or underscore word to the left.")
{
    seek_alpha_numeric_underscore(app, Scan_Backward);
}

CUSTOM_COMMAND_SIG(seek_alpha_numeric_underscore_right)
CUSTOM_DOC("Seek the right boundary of the nearest alpha-numeric or underscore word to the right.")
{
    seek_alpha_numeric_underscore(app, Scan_Forward);
}

CUSTOM_COMMAND_SIG(cut_word)
CUSTOM_DOC("Cut the word under the cursor.")
{
	seek_alpha_numeric_underscore_left(app);
	set_mark(app);
	seek_alpha_numeric_underscore_right(app);
	cut(app);
}

CUSTOM_COMMAND_SIG(copy_word)
CUSTOM_DOC("Copy the word under the cursor.")
{
	seek_alpha_numeric_underscore_left(app);
	set_mark(app);
	seek_alpha_numeric_underscore_right(app);
	copy(app);
}

CUSTOM_COMMAND_SIG(replace_word)
CUSTOM_DOC("Replaces the word under the cursor.")
{
	seek_alpha_numeric_underscore_left(app);
	set_mark(app);
	seek_alpha_numeric_underscore_right(app);
	replace_range_with_paste(app);
}

CUSTOM_COMMAND_SIG(backspace_alpha_numeric_underscore)
CUSTOM_DOC("Delete the underscore word to the left.")
{
	set_mark(app);
	seek_alpha_numeric_underscore_left(app);
	cut(app);
}

CUSTOM_COMMAND_SIG(delete_alpha_numeric_underscore)
CUSTOM_DOC("Delete the underscore word to the right.")
{
	set_mark(app);
	seek_alpha_numeric_underscore_right(app);
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

//---Panel interaction
CUSTOM_COMMAND_SIG(close_other_panel)
CUSTOM_DOC("Closes the inactive panel (fullscreens current panel).")
{
	change_active_panel(app);
	close_panel(app);
}

CUSTOM_COMMAND_SIG(view_current_buffer_in_two_panels)
CUSTOM_DOC("Ensures that two panels are open both containing the currently active buffer.")
{
    auto active_view_id = get_active_view(app, Access_ReadVisible);
    while (auto next_view_id = get_view_next(app, active_view_id, Access_ReadVisible)) {
        view_close(app, next_view_id);
    }
    open_panel_vsplit(app);
}
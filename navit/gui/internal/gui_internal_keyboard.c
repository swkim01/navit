#include <glib.h>
#include <stdlib.h>
#include "config.h"
#ifdef HAVE_LIBHANGUL
#include <hangul-1.0/hangul.h>
#endif
#include "color.h"
#include "coord.h"
#include "point.h"
#include "callback.h"
#include "graphics.h"
#include "debug.h"
#include "gui_internal.h"
#include "gui_internal_widget.h"
#include "gui_internal_priv.h"
#include "gui_internal_menu.h"
#include "gui_internal_keyboard.h"


static void
gui_internal_cmd_keypress(struct gui_priv *this, struct widget *wm, void *data)
{
	struct menu_data *md;
#ifdef HAVE_LIBHANGUL
	md=gui_internal_menu_data(this);
	if (md->keyboard_mode == 58) // Korean
		gui_internal_keypress_do_hangul(this, (char *) wm->data);
	else
#endif
		gui_internal_keypress_do(this, (char *) wm->data);
	md=gui_internal_menu_data(this);
	// Switch to lowercase after the first key is pressed
	if (md->keyboard_mode == 2) // Latin
		gui_internal_keyboard_do(this, md->keyboard, 10);
	if (md->keyboard_mode == 26) // Umlaut
		gui_internal_keyboard_do(this, md->keyboard, 34);
	if (md->keyboard_mode == 42) // Russian/Ukrainian/Belorussian
		gui_internal_keyboard_do(this, md->keyboard, 50);
#ifdef HAVE_LIBHANGUL
	//if (md->keyboard_mode == 58) // Korean
	//	gui_internal_keyboard_do(this, md->keyboard, 66);
#endif
}
	
static struct widget *
gui_internal_keyboard_key_data(struct gui_priv *this, struct widget *wkbd, char *text, int font, void(*func)(struct gui_priv *priv, struct widget *widget, void *data), void *data, void (*data_free)(void *data), int w, int h)
{
	struct widget *wk;
	gui_internal_widget_append(wkbd, wk=gui_internal_button_font_new_with_callback(this, text, font,
		NULL, gravity_center|orientation_vertical, func, data));
	wk->data_free=data_free;
	wk->background=this->background;
	wk->bl=0;
	wk->br=0;
	wk->bt=0;
	wk->bb=0;
	wk->w=w;
	wk->h=h;
	return wk;
}

static struct widget *
gui_internal_keyboard_key(struct gui_priv *this, struct widget *wkbd, char *text, char *key, int w, int h)
{
	return gui_internal_keyboard_key_data(this, wkbd, text, 0, gui_internal_cmd_keypress, g_strdup(key), g_free_func,w,h);
}

static void gui_internal_keyboard_change(struct gui_priv *this, struct widget *key, void *data);


// A list of availiable keyboard modes.
struct gui_internal_keyb_mode {
    char title[16]; // Label to be displayed on keys that switch to it
    int font; // Font size of label
    int case_mode; // Mode to switch to when case CHANGE() key is pressed.
    int umlaut_mode;  // Mode to switch to when UMLAUT() key is pressed.
} gui_internal_keyb_modes[]= {
	/* 0*/ {"ABC", 2,  8, 24},
	/* 8*/ {"abc", 2,  0, 32},
	/*16*/ {"123", 2,  0, 24},
	/*24*/ {"ÄÖÜ", 2, 40, 0},
	/*32*/ {"äöü", 2, 32, 8},
	/*40*/ {"АБВ", 2, 48,  0},
	/*48*/ {"абв", 2, 40,  8},
#ifdef HAVE_LIBHANGUL
	/*56*/ {"한글", 2, 56,  8},
#endif
	/*64*/ {"DEG", 2, 2,  2}
};


// Some macros that make the keyboard layout easier to visualise in
// the source code. The macros are #undef'd after this function.
#define KEY(x) gui_internal_keyboard_key(this, wkbd, (x), (x), max_w, max_h)
#define SPACER() gui_internal_keyboard_key_data(this, wkbd, "", 0, NULL, NULL, NULL,max_w,max_h)
#define MODE(x) gui_internal_keyboard_key_data(this, wkbd, \
		gui_internal_keyb_modes[(x)/8].title, \
		gui_internal_keyb_modes[(x)/8].font, \
		gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h) \
			-> datai=(mode&7)+((x)&~7)
#define SWCASE() MODE(gui_internal_keyb_modes[mode/8].case_mode)
#define UMLAUT() MODE(gui_internal_keyb_modes[mode/8].umlaut_mode)
struct widget *
gui_internal_keyboard_do(struct gui_priv *this, struct widget *wkbdb, int mode)
{
	struct widget *wkbd,*wk;
	struct menu_data *md=gui_internal_menu_data(this);
	int i, max_w=this->root.w, max_h=this->root.h;
	int render=0;
	char *space="_";
	char *backspace="←";
	char *hide="▼";
	char *unhide="▲";

	if (wkbdb) {
		this->current.x=-1;
		this->current.y=-1;
		gui_internal_highlight(this);
		if (md->keyboard_mode >= 1024)
			render=2;
		else
			render=1;
		gui_internal_widget_children_destroy(this, wkbdb);
	} else
		wkbdb=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_fill);
	md->keyboard=wkbdb;
	md->keyboard_mode=mode;
	wkbd=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_fill);
	wkbd->background=this->background;
	wkbd->cols=8;
	wkbd->spx=0;
	wkbd->spy=0;
	max_w=max_w/8;
	max_h=max_h/8; // Allows 3 results in the list when searching for Towns
	wkbd->p.y=max_h*2;
#ifdef HAVE_LIBHANGUL
	struct widget *wi,*menu;
	menu=g_list_last(this->root.children)->data;
        wi=gui_internal_find_widget(menu, NULL, STATE_EDIT);
	gui_internal_hic_flush(this, wi);
	if(mode>=40&&mode<64) { // Russian/Ukrainian/Belarussian/Korean layout needs more space...
#else
	if(mode>=40&&mode<56) { // Russian/Ukrainian/Belarussian layout needs more space...
#endif
		max_h=max_h*4/5;
		max_w=max_w*8/9;
		wkbd->cols=9;
	}

	if (mode >= 0 && mode < 8) {
		for (i = 0 ; i < 26 ; i++) {
			char text[]={'A'+i,'\0'};
			KEY(text);
		}
		gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);
		if (mode == 0) {
			KEY("-");
			KEY("'");
			wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
			wk->datai=mode+1024;
		} else {
			wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
			wk->datai=mode+1024;
			SWCASE();
			MODE(16);
			
		}
		UMLAUT();
		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}
	if (mode >= 8 && mode < 16) {
		for (i = 0 ; i < 26 ; i++) {
			char text[]={'a'+i,'\0'};
			KEY(text);
		}
		gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);
		if (mode == 8) {
			KEY("-");
			KEY("'");
			wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
			wk->datai=mode+1024;
		} else {
			wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
			wk->datai=mode+1024;
			SWCASE();
			
			MODE(16);
		}
		UMLAUT();
		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}
	if (mode >= 16 && mode < 24) {
		for (i = 0 ; i < 10 ; i++) {
			char text[]={'0'+i,'\0'};
			KEY(text);
		}
		/* ("8")     ("9")*/KEY("."); KEY("°"); KEY("'"); KEY("\"");KEY("-"); KEY("+");
		KEY("*"); KEY("/"); KEY("("); KEY(")"); KEY("="); KEY("?"); KEY(":"); SPACER();

		

		if (mode == 16) {
			SPACER();
			KEY("-");
			KEY("'");
			wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
			wk->datai=mode+1024;
			SPACER();
			SPACER();
		} else {
			SPACER();
#ifdef HAVE_LIBHANGUL
			if (strstr("KR",this->country_iso2?this->country_iso2:getenv("LANG"))) {
				SPACER();
				MODE(56);
			}
			else {
				MODE(40);
				MODE(48);
			}
#else
			MODE(40);
			MODE(48);
#endif
			wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
			wk->datai=mode+1024;
			MODE(0);
			MODE(8);
		}
		UMLAUT();
		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}
	if (mode >= 24 && mode < 32) {
		KEY("Ä"); KEY("Ë"); KEY("Ï"); KEY("Ö"); KEY("Ü"); KEY("Æ"); KEY("Ø"); KEY("Å");
		KEY("Á"); KEY("É"); KEY("Í"); KEY("Ó"); KEY("Ú"); KEY("Š"); KEY("Č"); KEY("Ž");
		KEY("À"); KEY("È"); KEY("Ì"); KEY("Ò"); KEY("Ù"); KEY("Ś"); KEY("Ć"); KEY("Ź");
		KEY("Â"); KEY("Ê"); KEY("Î"); KEY("Ô"); KEY("Û"); SPACER();

		UMLAUT();

		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}
	if (mode >= 32 && mode < 40) {
		KEY("ä"); KEY("ë"); KEY("ï"); KEY("ö"); KEY("ü"); KEY("æ"); KEY("ø"); KEY("å");
		KEY("á"); KEY("é"); KEY("í"); KEY("ó"); KEY("ú"); KEY("š"); KEY("č"); KEY("ž");
		KEY("à"); KEY("è"); KEY("ì"); KEY("ò"); KEY("ù"); KEY("ś"); KEY("ć"); KEY("ź");
		KEY("â"); KEY("ê"); KEY("î"); KEY("ô"); KEY("û"); KEY("ß");

		UMLAUT();

		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}
	if (mode >= 40 && mode < 48) {
		KEY("А"); KEY("Б"); KEY("В"); KEY("Г"); KEY("Д"); KEY("Е"); KEY("Ж"); KEY("З"); KEY("И");
		KEY("Й"); KEY("К"); KEY("Л"); KEY("М"); KEY("Н"); KEY("О"); KEY("П"); KEY("Р"); KEY("С");
		KEY("Т"); KEY("У"); KEY("Ф"); KEY("Х"); KEY("Ц"); KEY("Ч"); KEY("Ш"); KEY("Щ"); KEY("Ъ"); 
		KEY("Ы"); KEY("Ь"); KEY("Э"); KEY("Ю"); KEY("Я"); KEY("Ё"); KEY("І"); KEY("Ї"); KEY("Ў");
		SPACER(); SPACER(); SPACER();
		gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);

		wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
		wk->datai=mode+1024;

		SWCASE();

		MODE(16);

		SPACER();

		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}
	if (mode >= 48 && mode < 56) {
		KEY("а"); KEY("б"); KEY("в"); KEY("г"); KEY("д"); KEY("е"); KEY("ж"); KEY("з"); KEY("и");
		KEY("й"); KEY("к"); KEY("л"); KEY("м"); KEY("н"); KEY("о"); KEY("п"); KEY("р"); KEY("с");
		KEY("т"); KEY("у"); KEY("ф"); KEY("х"); KEY("ц"); KEY("ч"); KEY("ш"); KEY("щ"); KEY("ъ");
		KEY("ы"); KEY("ь"); KEY("э"); KEY("ю"); KEY("я"); KEY("ё"); KEY("і"); KEY("ї"); KEY("ў");
		SPACER(); SPACER(); SPACER();
		gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);
		
		wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
		wk->datai=mode+1024;

		SWCASE();

		MODE(16);

		SPACER();

		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}

#ifdef HAVE_LIBHANGUL
	if (mode >= 56 && mode < 64) {
		KEY("ㄱ"); KEY("ㄲ"); KEY("ㄴ"); KEY("ㄷ"); KEY("ㄸ"); KEY("ㄹ"); KEY("ㅁ"); KEY("ㅂ"); KEY("ㅃ");
		KEY("ㅅ"); KEY("ㅆ"); KEY("ㅇ"); KEY("ㅈ"); KEY("ㅉ"); KEY("ㅊ"); KEY("ㅋ"); KEY("ㅌ"); KEY("ㅍ");
		KEY("ㅎ"); KEY("ㅏ"); KEY("ㅐ"); KEY("ㅑ"); KEY("ㅒ"); KEY("ㅓ"); KEY("ㅔ"); KEY("ㅕ"); KEY("ㅖ");
		KEY("ㅗ"); KEY("ㅚ"); KEY("ㅛ"); KEY("ㅜ"); KEY("ㅟ"); KEY("ㅠ"); KEY("ㅡ"); KEY("ㅢ"); KEY("ㅣ");
		SPACER(); SPACER(); SPACER();
		gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);
		
		wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
		wk->datai=mode+1024;

		SWCASE();

		MODE(16);

		SPACER();

		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}
#endif


	if(md->search_list && md->search_list->type==widget_table) {
		struct table_data *td=(struct table_data*)(md->search_list->data);
		td->scroll_buttons.button_box_hide=mode<1024;
	}

#ifdef HAVE_LIBHANGUL
	if (mode >= 64 && mode < 72) { /* special case for coordinates input screen (enter_coord) */
#else
	if (mode >= 56 && mode < 64) { /* special case for coordinates input screen (enter_coord) */
#endif
		KEY("0"); KEY("1"); KEY("2"); KEY("3"); KEY("4"); SPACER(); KEY("N"); KEY("S");
		KEY("5"); KEY("6"); KEY("7"); KEY("8"); KEY("9"); SPACER(); KEY("E"); KEY("W");
		KEY("°"); KEY("."); KEY("'"); 
		gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);
		SPACER();

		wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
		wk->datai=mode+1024;

		SPACER();
		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}	

	if (mode >= 1024) {
		char *text=NULL;
		int font=0;
		struct widget *wkl;
		mode -= 1024;
		text=gui_internal_keyb_modes[mode/8].title;
		font=gui_internal_keyb_modes[mode/8].font;
		wk=gui_internal_box_new(this, gravity_center|orientation_horizontal|flags_fill);
		wk->func=gui_internal_keyboard_change;
		wk->data=wkbdb;
		wk->background=this->background;
		wk->bl=0;
		wk->br=0;
		wk->bt=0;
		wk->bb=0;
		wk->w=max_w;
		wk->h=max_h;
		wk->datai=mode;
		wk->state |= STATE_SENSITIVE;
		gui_internal_widget_append(wk, wkl=gui_internal_label_new(this, unhide));
		wkl->background=NULL;
		gui_internal_widget_append(wk, wkl=gui_internal_label_font_new(this, text, font));
		wkl->background=NULL;
		gui_internal_widget_append(wkbd, wk);
		if (render)
			render=2;
	}
	gui_internal_widget_append(wkbdb, wkbd);
	if (render == 1) {
		gui_internal_widget_pack(this, wkbdb);
		gui_internal_widget_render(this, wkbdb);
	} else if (render == 2) {
		gui_internal_menu_reset_pack(this);
		gui_internal_menu_render(this);
	}
	return wkbdb;
}
#undef KEY
#undef SPACER
#undef SWCASE
#undef UMLAUT
#undef MODE

struct widget *
gui_internal_keyboard(struct gui_priv *this, int mode)
{
	if (! this->keyboard)
		return NULL;
	return gui_internal_keyboard_do(this, NULL, mode);
}

static void
gui_internal_keyboard_change(struct gui_priv *this, struct widget *key, void *data)
{
	gui_internal_keyboard_do(this, key->data, key->datai);
}
int
gui_internal_keyboard_init_mode(char *lang)
{
	int ret=0;
	/* Converting to upper case here is required for Android */
	lang=g_ascii_strup(lang,-1);
	/*
	* Set cyrillic keyboard for countries using Cyrillic alphabet
	*/
	if (strstr(lang,"RU"))
	    ret = 40;
	if (strstr(lang,"UA"))
	    ret = 40;
	if (strstr(lang,"BY"))
	    ret = 40;
	if (strstr(lang,"RS"))
	    ret = 40;
	if (strstr(lang,"BG"))
	    ret = 40;
	if (strstr(lang,"MK"))
	    ret = 40;
	if (strstr(lang,"KZ"))
	    ret = 40;
	if (strstr(lang,"KG"))
	    ret = 40;
	if (strstr(lang,"TJ"))
	    ret = 40;
	if (strstr(lang,"MN"))
	    ret = 40;
#ifdef HAVE_LIBHANGUL
	if (strstr(lang,"KR"))
	    ret = 56;
#endif
	g_free(lang);
	return ret;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mz.h"

char *resdir_get_typestring(RESDIR *self)
{
	static char buf[16];
	
	switch ((*self->get_type)(self))
	{
		case RT_CURSOR:		return "Cursor";
		case RT_BITMAP:		return "Bitmap";
		case RT_ICON:		return "Icon";
		case RT_MENU:		return "Menu";
		case RT_DIALOG:		return "Dialog";
		case RT_STRING:		return "String";
		case RT_FONTDIR:	return "FontDir";
		case RT_FONT:		return "Font";
		case RT_ACCELERATOR:	return "Accelerator";
		case RT_RCDATA:		return "RcData";
		case RT_MESSAGELIST:	return "MessageList";
		case RT_GROUP_CURSOR:	return "CursorGroup";
		case RT_GROUP_ICON:	return "IconGroup";
	}	
	sprintf(buf, "(%x)", (*self->get_type)(self));
	return buf;
}





char *resdirentry_get_typestring(RESDIRENTRY *self)
{
        static char buf[16];
        
	switch ((*self->get_type)(self))
        {
                case RT_CURSOR:         return "Cursor";
                case RT_BITMAP:         return "Bitmap";
                case RT_ICON:           return "Icon";
                case RT_MENU:           return "Menu";
                case RT_DIALOG:         return "Dialog";
                case RT_STRING:         return "String";
                case RT_FONTDIR:        return "FontDir";
                case RT_FONT:           return "Font";
                case RT_ACCELERATOR:    return "Accelerator";
                case RT_RCDATA:         return "RcData";
                case RT_MESSAGELIST:    return "MessageList";
                case RT_GROUP_CURSOR:   return "CursorGroup";
                case RT_GROUP_ICON:     return "IconGroup";
        }
        sprintf(buf, "(%x)", (*self->get_type)(self));
        return buf;
}


RESDIRENTRY *resdir_find_name(RESDIR *self, const char *s)
{
        int blen  = 1 + strlen(s);
        char *buf = malloc(blen);
        RESDIRENTRY *re2, *re;

	re = (*self->find_first)(self);
        while (re)
        {
                (*re->get_name)(re, buf, blen);
                if (!strcmp(buf, s))
                {
                 	free(buf);
                        return re;
                }
                re2 = re;
                re = (self->find_next)(self, re2);
		free(re2);
        }
	free(buf);
	return NULL;
}



RESDIRENTRY *resdir_find_id(RESDIR *self, int id)
{
        RESDIRENTRY *re, *re2;

	re = (self->find_first)(self);
        while (re)
        {
                if ((re->get_id)(re) == id) return re;
                re2 = re;
                re = (self->find_next)(self, re2);
                free(re2);
        }
        return NULL;
}




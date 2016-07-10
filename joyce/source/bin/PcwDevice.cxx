/************************************************************************

    JOYCE v2.1.0 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2  John Elliott <seasip.webmaster@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*************************************************************************/

#include "Pcw.hxx"

PcwDevice::PcwDevice(const string sClass, const string sName)
{
	string s = "PcwDevice::PcwDevice ("+sClass+","+sName+")";
	XLOG(s.c_str());
	m_enabled = true;
	m_class = sClass;
	m_name = sName;
	m_model = PCW8000;
	
}


PcwDevice::~PcwDevice()
{
}


bool PcwDevice::parse(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr parent)
{
	xmlNodePtr cur;

	// No XML settings to read
	if (m_class == "") return true;

	// Find a node that looks like <class name="foo">
	for (cur = parent->xmlChildrenNode; cur; cur = cur->next)
	{
		if (m_class.compare((char *)cur->name) || 
                    (cur->ns != ns)) continue;

		char *s = (char *)xmlGetProp(cur, (xmlChar *)"name");
		if (m_name != "")
		{
			if (!s) continue;
			if (m_name.compare(s)) 
			{
				xmlFree(s);
				continue;
			}
		}
		if (s) xmlFree(s);
	
		return parseNode(doc, ns, cur);
	}
	return true;	// No node found, OK to use defaults
}
//
// Save settings to JOYCEHW.XML
//
bool PcwDevice::store(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr parent)
{
	bool saved, found;

	// No XML settings to read
	if (m_class == "") return true;

	xmlNode *cur;

	saved = found = false;
	// Find a node that looks like <class name="foo">
	for (cur = parent->xmlChildrenNode; cur; cur = cur->next)
	{
		if (m_class.compare((char *)cur->name) || 
                    (cur->ns != ns)) continue;

		char *s = (char *)xmlGetProp(cur, (xmlChar *)"name");
		if (m_name != "")
		{
			if (!s) continue;
			if (m_name.compare(s)) 
			{
				xmlFree(s);
				continue;
			}
		}
		if (s) xmlFree(s);
		found = true;	
		saved |= storeNode(doc, ns, cur);
	}
	if (!found) // Node not found. Create it.
	{
		cur = xmlNewNode(ns, (xmlChar *)m_class.c_str());
		if (m_name != "") xmlSetProp(cur, (xmlChar *)"name", (xmlChar *)m_name.c_str());
		saved = storeNode(doc, ns, cur);
		if (saved) xmlAddChild(parent, cur);
	}
	return saved;
}


bool PcwDevice::parseEnabled(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char *s = (char *)xmlGetProp(cur, (xmlChar *)"enabled");

	if (!s) return true;	// Default setting used

	if (!strcmp(s, "1")) { enable(true);  xmlFree(s); return true; }
	if (!strcmp(s, "0")) { enable(false); xmlFree(s); return true; }

	joyce_dprintf("'enabled=%s' option not recognised.\n", s);	
	xmlFree(s);
       	return false;
}


bool PcwDevice::storeEnabled(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	xmlSetProp(cur, (xmlChar *)"enabled", (xmlChar *)(isEnabled() ? "1" : "0"));
	return true;
}

bool PcwDevice::parseFilename(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, 
				const string nodename, string &name)
{
	for (cur = cur->xmlChildrenNode; cur ; cur = cur->next)
	{
		if (cur->ns != ns) continue;
		if (nodename.compare((char *)cur->name)) continue;
		char *t = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
                if (t)  /* v2.0.1: t may be null */
                {
                        name = t;
                        xmlFree(t);
                }
                else    name = "";
	}
	return true;
}

			
bool PcwDevice::storeFilename(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, 
				const string nodename, string name)
{
	bool found = false;
        xmlNodePtr output, newOutput, oFilename;

        newOutput    = xmlNewNode(ns, (xmlChar *)nodename.c_str());
        oFilename    = xmlNewText((xmlChar *)name.c_str());
	xmlAddChild(newOutput, oFilename);
	
	// Replace any child nodes with name matching "nodename"

        for (output = cur->xmlChildrenNode; output; output = output->next)
        {
                if (output->ns != ns) continue;
                if (nodename.compare((char *)output->name)) continue;

                found = true;
                xmlReplaceNode(output, newOutput);
        }
	// None found - create one.
        if (!found) xmlAddChild(cur, newOutput);
        return true;
}

bool PcwDevice::storeInteger(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, 
				const string nodename, int i)
{
	char buf[20];
	bool found = false;
        xmlNodePtr output, newOutput, oValue;

	sprintf(buf, "%d", i);

        newOutput    = xmlNewNode(ns, (xmlChar *)nodename.c_str());
        oValue       = xmlNewText((xmlChar *)buf);
	xmlAddChild(newOutput, oValue);
	
	// Replace any child nodes with name matching "nodename"

        for (output = cur->xmlChildrenNode; output; output = output->next)
        {
                if (output->ns != ns) continue;
                if (nodename.compare((char *)output->name)) continue;

                found = true;
                xmlReplaceNode(output, newOutput);
        }
	// None found - create one.
        if (!found) xmlAddChild(cur, newOutput);
        return true;
}


bool PcwDevice::parseInteger(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, 
				const string nodename, int *i)
{
	for (cur = cur->xmlChildrenNode; cur ; cur = cur->next)
	{
		if (cur->ns != ns) continue;
		if (nodename.compare((char *)cur->name)) continue;
		char *t = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if (isdigit(t[0])) *i = atoi(t);
		free(t);
	}
	return true;
}




bool PcwDevice::isEnabled(void)
{
	return m_enabled;
}

void PcwDevice::enable(bool b)
{
	m_enabled = b;
}


bool PcwDevice::parseColour(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur,
                           const string title, SDL_Colour &fg, SDL_Colour &bg)
{
        for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
        {
                if (cur->ns != ns) continue;
                char *t = (char *)xmlGetProp(cur, (xmlChar *)"name");
                if (t && !title.compare(t))
                {
                        parseColour(cur, "fg", fg);
                        parseColour(cur, "bg", bg);
                }
                if (t) xmlFree(t);
        }
	return true;
}


bool PcwDevice::storeColour(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur,
                          const string title, SDL_Colour &fg, SDL_Colour &bg)
{
        int d = 0;
        xmlNodePtr o = cur;

        for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
        {
                if (cur->ns != ns) continue;
                if (strcmp((char *)cur->name, "colour")) continue;
                char *t = (char *)xmlGetProp(cur, (xmlChar *)"name");
                if (t && !title.compare(t))
                {
                        putColour(cur, "fg", fg);
                        putColour(cur, "bg", bg);
                        d = 1;
                }
                if (t) xmlFree(t);
        }
        if (!d)
	{
                cur = xmlNewNode(ns, (xmlChar *)"colour");
                putColour(cur, "fg", fg);
                putColour(cur, "bg", bg);
		xmlSetProp(cur, (xmlChar *)"name", (xmlChar *)title.c_str());
                xmlAddChild(o, cur);
        }
	return true;
}

void PcwDevice::putColour(xmlNodePtr cur, const string title, SDL_Colour &clr)
{
        char buf[20];

        sprintf(buf, "#%02x%02x%02x", clr.r, clr.g, clr.b);
        xmlSetProp(cur, (xmlChar *)title.c_str(), (xmlChar *)buf);
}

static int xtoi(const char *s)
{
        int hexv = 0;

        do
        {
                char c = toupper(*s);
                if (isdigit(c))                hexv = (hexv * 16) + (c - '0');
                else if (c >= 'A' && c <= 'F') hexv = (hexv * 16) + (c - 'A' + 10);
                else return hexv;
                ++s;
        } while (1);
        return 0;
}

// Parse a colour attribute of the form #rgb or #rrggbb 
void PcwDevice::parseColour(xmlNodePtr node, const string attr, SDL_Colour &clr)
{
        char *s = (char *)xmlGetProp(node, (const xmlChar *)attr.c_str());
        char r[3], g[3], b[3];

        if (!s) return;

        if (s[0] != '#' || (strlen(s) != 4 && strlen(s) != 7))
        {
                joyce_dprintf("Invalid colour: %s\n", s);
                xmlFree(s);
        }
        if (strlen(s) == 4)
        {
                sprintf(r, "%c0", s[1]);
                sprintf(g, "%c0", s[2]);
                sprintf(b, "%c0", s[3]);
        }
        else
        {
                sprintf(r, "%-2.2s", s+1);
                sprintf(g, "%-2.2s", s+3);
                sprintf(b, "%-2.2s", s+5);
        }
        clr.r = xtoi(r); clr.g = xtoi(g); clr.b = xtoi(b);
        xmlFree(s);
}


//////////////////////////////////////////////////////////////////////////
//
// Support for settings
//
bool PcwDevice::hasSettings(SDLKey *key, string &caption)
{
	return false;
}

//
// Display settings screen
//
UiEvent PcwDevice::settings(UiDrawer *d)
{
	return UIE_CONTINUE;
}


int PcwDevice::handleEvent(SDL_Event &e)
{
	if (e.type == SDL_QUIT) return -99;
	return 0;
}

//
// Set any behaviour that depends on PCW model
//
void PcwDevice::setModel(PcwModel j)
{
	m_model = j;
}

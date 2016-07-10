/* PCW Explorer - access Amstrad PCW discs on Linux or Windows
    Copyright (C) 2000  John Elliott

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
*/
class PcwGraphicFactory : public PcwFileFactory
{
public:
	PcwGraphicFactory();

	/* Overridables: */

	/* If not overridden, loads the whole image just to get its size */
        virtual wxSize GetDimensions(const char *filename);

	/* Shouldn't need to override this if you've registered a */
	/* wxImageHandler */
	virtual wxImage *GetImage(const char *filename);
	/* Get the ID of the image type we registered */
	virtual int GetImageType(void);

        virtual wxWindow *GetViewport(wxDialog *parent, const char *filename);

	virtual void AddPages(wxNotebook *notebook, const char *filename);

        // Build a context menu for this type of file
        virtual void BuildContextMenu(wxMenu &);
        // Context menu events
        virtual void OnCtx(wxFrame *frame, int menuid, const char *filename);
protected:
	void ExportImage(int format, const char *filename);
};





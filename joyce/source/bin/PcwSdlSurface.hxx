
class PcwSdlSurface
{
protected:
	SDL_Surface *m_surface;
	int m_mustlock;
public:
	PcwSdlSurface(SDL_Surface *attach);
	PcwSdlSurface(int width, int height, int bpp, int flags);
	~PcwSdlSurface();
	int lock(void);
	void unlock(void);

	// Accessors 
	inline SDL_Surface *getSurface(void) { return m_surface; }
	inline int getWidth () { return m_surface->w; }
	inline int getHeight() { return m_surface->h; }
	inline int getPitch()  { return m_surface->pitch; }
	inline int getBPP()    { return m_surface->format->BitsPerPixel; }
	inline int isFull()    { return m_surface->flags & SDL_FULLSCREEN; }
	inline int isHardware(){ return m_surface->flags & SDL_HWSURFACE; } 

	void setColours(SDL_Colour *colors, int firstcolor, int ncolors);
};


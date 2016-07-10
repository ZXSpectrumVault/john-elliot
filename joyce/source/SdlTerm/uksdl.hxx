/* This saves me being schizophrenic about American spelling */

#ifndef SDL_Colour
#define SDL_Colour SDL_Color    
#endif				

/* v2.1.5: SDL headers now define SDL_Colour but not SDL_SetColours. So 
 * putting both in the one #ifndef stops working. */

#ifndef SDL_SetColours
#define SDL_SetColours SDL_SetColors
#endif


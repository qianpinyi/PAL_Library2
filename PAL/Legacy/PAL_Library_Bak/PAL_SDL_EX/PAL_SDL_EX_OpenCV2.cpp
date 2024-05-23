#ifndef PAL_SDL_EX_OPENCV2
#define PAL_SDL_EX_OPENCV2 1

#include "../PAL_BuildConfig.h"
#include PAL_SDL_HeaderPath
#include PAL_OpenCV2_matHeaderPath

SDL_Texture* CreateSDLTextureFromOpenCVImage(SDL_Renderer *ren,const cv::Mat &src)
{
	if (src.empty()||src.cols<=0||src.rows<=0||src.dims!=2) return NULL;
	SDL_Surface *sur=SDL_CreateRGBSurfaceWithFormatFrom((void*)src.data,src.cols,src.rows,24,src.cols*3,SDL_PIXELFORMAT_BGR24);
	if (sur==NULL) return NULL; 
	SDL_Texture *tex=SDL_CreateTextureFromSurface(ren,sur);
	SDL_FreeSurface(sur);
	return tex;
}

SDL_Surface* CreateSDLSurfaceFromOpenCVImage(const cv::Mat &src)
{
	if (src.empty()||src.cols<=0||src.rows<=0||src.dims!=2) return NULL;
	SDL_Surface *sur=SDL_CreateRGBSurfaceWithFormatFrom((void*)src.data,src.cols,src.rows,24,src.cols*3,SDL_PIXELFORMAT_BGR24);
	SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
	SDL_Surface *re=SDL_ConvertSurface(sur,format,0);
	SDL_FreeFormat(format);
	SDL_FreeSurface(sur);
	return re;
}

#endif

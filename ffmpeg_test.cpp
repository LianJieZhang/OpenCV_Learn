//gcc simplest_ffmpeg_player.c -g -o smp.out -lSDLmain -lSDL -lavformat -lavcodec -lavutil -lswscale
#include <stdio.h>
#define __STDC_CONSTANT_MACROS 
extern "C"
{
#include <sys/time.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <SDL/SDL.h> 
#include <sys/shm.h> 
};

#define SHOW_FULLSCREEN 0
#define OUTPUT_YUV420P 0  

#define SHM_SIZE 4 * 1024
#define SHM_MODE 0666 | IPC_CREAT
int main(int argc, char* argv[])  
{
		//shm
		int shmid;  
		//opencv
		//Stats RunTime
		struct timeval tpstart, tpend;
		float timeuse;
		gettimeofday(&tpstart, NULL);
		//FFmpeg  
		AVStream *video_st = NULL;
		AVFormatContext *pFormatCtx;  
		int             i, videoindex;  
		AVCodecContext  *pCodecCtx;  
		AVCodec         *pCodec;  
		AVFrame *pFrame,*pFrameYUV;  
		AVPacket *packet;  
		struct SwsContext *img_convert_ctx;  
		//SDL  
		int screen_w,screen_h;  
		SDL_Surface *screen;   
		SDL_VideoInfo *vi;  
		SDL_Overlay *bmp;   
		SDL_Rect rect;  
		FILE *fp_yuv;  
		int ret, got_picture;  
		char filepath[1024]={'\0'};
		key_t key;
		if ((key = ftok("/dev/null",66)) < -1) {
            printf(" ftok faild\n");
            return -1;
        }	
		if ((shmid = shmget(key, SHM_SIZE,SHM_MODE)) < 0) {
			printf("shmget() faild\n");
			return -1;
		}
			
		memcpy(filepath,argv[1],strlen(argv[1]));  
		av_register_all();  
		avformat_network_init();  

		pFormatCtx = avformat_alloc_context(); 

		if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0){  
				printf("Couldn't open input stream.\n");  
				return -1;  
		}  
		if(avformat_find_stream_info(pFormatCtx,NULL)<0){  
				printf("Couldn't find stream information.\n");  
				return -1;  
		}

		printf("pFormatCtx->nb_streams:%d\n",pFormatCtx->nb_streams);
		videoindex=-1;  
		for(i=0; i<pFormatCtx->nb_streams; i++)   
				if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){  
						videoindex=i;  
						break;  
				}  
		if(videoindex==-1){  
				printf("Didn't find a video stream.\n");  
				return -1;  
		}  

		pCodecCtx=pFormatCtx->streams[videoindex]->codec;  

		pCodec=avcodec_find_decoder(pCodecCtx->codec_id);  

		if(pCodec==NULL){  
				printf("Codec not found.\n");  
				return -1;  
		}  
		if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){  
				printf("Could not open codec.\n");  
				return -1;  
		}  
		video_st = avformat_new_stream(pFormatCtx, pCodec);



		pFrame=av_frame_alloc();  
		pFrameYUV=av_frame_alloc();  
		//uint8_t *out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));  
		//avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);  
		//SDL----------------------------  
		if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {    
				printf( "Could not initialize SDL - %s\n", SDL_GetError());   
				return -1;  
		}   
#if SHOW_FULLSCREEN  
		vi = SDL_GetVideoInfo();  
		screen_w = vi->current_w;  
		screen_h = vi->current_h;  
		screen = SDL_SetVideoMode(screen_w, screen_h, 0,SDL_FULLSCREEN);  
#else  
		screen_w = pCodecCtx->width;  
		screen_h = pCodecCtx->height;  
		screen = SDL_SetVideoMode(screen_w, screen_h, 0,0);  
#endif  
		if(!screen) {    
				printf("SDL: could not set video mode - exiting:%s\n",SDL_GetError());    
				return -1;  
		}  

		bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height,SDL_YV12_OVERLAY, screen);   

		rect.x = 0;      
		rect.y = 0;      
		rect.w = screen_w;      
		rect.h = screen_h;    
		//SDL End------------------------  
		packet=(AVPacket *)av_malloc(sizeof(AVPacket));  
		//Output Information-----------------------------  
		printf("------------- File Information ------------------\n");  
		av_dump_format(pFormatCtx,0,filepath,0);  
		printf("-------------------------------------------------\n");  

#if OUTPUT_YUV420P   
		fp_yuv=fopen("output.yuv","wb+");    
#endif    

		SDL_WM_SetCaption("Simplest FFmpeg Player",NULL);  

		img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);   
		//------------------------------  
		double FirstTime = 0.0;
		double CurTime = 0.0;
		double TempTime = 0.0;
		double FinalTime = 0.0;
		char *TimeBuf = NULL;
		bool iFlag = false;
		int aFlag = 0;
		void *shmaddr = NULL;
		if ((shmaddr = shmat(shmid, NULL, 0)) == (void *)-1) {
			printf("shmaddr faild\n");
			return -1;
		}
		TimeBuf = (char *)shmaddr;
		
		while(av_read_frame(pFormatCtx, packet)>=0){ 
				if(packet->stream_index==videoindex){
						printf("----------------:%f\n",packet->pts * av_q2d(video_st->time_base)); 
						if (iFlag == false) {
								FirstTime = packet->pts * av_q2d(video_st->time_base);
								if (FirstTime > 0)
										iFlag = true;
						}
						if (packet->flags == 1 && aFlag == 0) {
								aFlag = 1;
						}
						CurTime = packet->pts * av_q2d(video_st->time_base) - FirstTime;
							
						if (TempTime <= CurTime) {
							FinalTime = TempTime;
							TempTime = CurTime;
						} else {
							FinalTime = CurTime;
						}
						printf("rela_time:%f\n",FinalTime);
						//memset(TimeBuf, 0, 1024);
						sprintf(TimeBuf,"%f,%f",FinalTime,timeuse);
						//printf("rela_time:%f\n", packet->pts * av_q2d(video_st->time_base));
						

						//Decode 
						ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);  
						if(ret < 0){  
								printf("Decode Error.\n");  
								return -1;  
						}  
						if(got_picture){  
								SDL_LockYUVOverlay(bmp);  
								pFrameYUV->data[0]=bmp->pixels[0];  
								pFrameYUV->data[1]=bmp->pixels[2];  
								pFrameYUV->data[2]=bmp->pixels[1];       
								pFrameYUV->linesize[0]=bmp->pitches[0];  
								pFrameYUV->linesize[1]=bmp->pitches[2];     
								pFrameYUV->linesize[2]=bmp->pitches[1];  
								sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,   
												pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);  
#if OUTPUT_YUV420P  
								int y_size=pCodecCtx->width*pCodecCtx->height;    
								fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y   
								fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U  
								fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V  
#endif  

								SDL_UnlockYUVOverlay(bmp);   

								SDL_DisplayYUVOverlay(bmp, &rect);   
								//Delay 40ms  
								//SDL_Delay(40);  
						}
						if (aFlag == 1) {
								gettimeofday(&tpend, NULL);			
								timeuse = (tpend.tv_sec - tpstart.tv_sec) + (tpend.tv_usec - tpstart.tv_usec) /1000000.0;
								printf("timeuse:%f\n",timeuse);
								aFlag = 2;
						}
				}  
				av_free_packet(packet);  
		}  
		//FIX: Flush Frames remained in Codec  
		while (1) {  
				printf("--------------------\n");
				ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);  
				if (ret < 0)  
						break;  
				if (!got_picture)  
						break;  
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);  

				SDL_LockYUVOverlay(bmp);  
				pFrameYUV->data[0]=bmp->pixels[0];  
				pFrameYUV->data[1]=bmp->pixels[2];  
				pFrameYUV->data[2]=bmp->pixels[1];       
				pFrameYUV->linesize[0]=bmp->pitches[0];  
				pFrameYUV->linesize[1]=bmp->pitches[2];     
				pFrameYUV->linesize[2]=bmp->pitches[1];  
#if OUTPUT_YUV420P  
				int y_size=pCodecCtx->width*pCodecCtx->height;    
				fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y   
				fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U  
				fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V  
#endif  

				SDL_UnlockYUVOverlay(bmp);   
				SDL_DisplayYUVOverlay(bmp, &rect);   
				//Delay 40ms  
				SDL_Delay(40);  
		}  

		sws_freeContext(img_convert_ctx);  

#if OUTPUT_YUV420P   
		fclose(fp_yuv);  
#endif   

		SDL_Quit();  

		//av_free(out_buffer);  
		av_free(pFrameYUV);  
		avcodec_close(pCodecCtx);  
		avformat_close_input(&pFormatCtx);  

		return 0;  
}  

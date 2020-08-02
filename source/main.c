/* 
   TINY3D sample / (c) 2010 Hermes  <www.elotrolado.net>

   PS3LoadX is the evolution of PSL1GHT PS3Load sample

*/

#include <lv2/process.h>
#include <psl1ght/lv2/filesystem.h>
#include <sys/stat.h>


#include <psl1ght/lv2/thread.h>
#include <sysmodule/sysmodule.h>

#include <sysutil/events.h>

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/net.h>

#include <zlib.h>
#include <zip.h>

#include "gfx.h"
#include "pad.h"

int v_release = 0;

char msg_error[128];
//char msg_one  [128];
char msg_two  [128];

char bootpath[MAXPATHLEN];

#define RGBA(r, g, b, a) (((r) << 24) | ((g) << 16) | ((b) << 8) | (a))

void release_all();

#define DT_DIR 1

#define VERSION "v1.1"
#define PORT 4299
#define MAX_ARG_COUNT 0x100

#define ERROR(a, msg) { \
	if (a < 0) { \
		sprintf(msg_error, "PS3LoadX: " msg ); \
        usleep(250); \
        if(my_socket >= 0) { close(my_socket);my_socket = -1;} \
	} \
}
#define ERROR2(a, msg) { \
	if (a < 0) { \
		sprintf(msg_error, "PS3LoadX: %s", msg ); \
        sleep(2); \
        msg_error[0] = 0; \
		usleep(60); \
		goto reloop; \
	} \
}
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ZIP_PATH "/dev_hdd0/tmp/ps3load"
#define SELF_PATH ZIP_PATH ".self"

char ps3load_path[MAXPATHLEN]= "/dev_hdd0/game/PSL145310/homebrew";
char install_folder[MAXPATHLEN];

void DeleteDirectory(const char* path)
{
	int dfd;
	u64 read;
	Lv2FsDirent dir;
	if (lv2FsOpenDir(path, &dfd))
		return;
    
    read = sizeof(Lv2FsDirent);
	while (!lv2FsReadDir(dfd, &dir, &read)) {
		char newpath[0x440];
		if (!read)
			break;
		if (!strcmp(dir.d_name, ".") || !strcmp(dir.d_name, ".."))
			continue;

		strcpy(newpath, path);
		strcat(newpath, "/");
		strcat(newpath, dir.d_name);

		if (dir.d_type & DT_DIR) {
			DeleteDirectory(newpath);
			rmdir(newpath);
		} else {
			remove(newpath);
		}
	}
	lv2FsCloseDir(dfd);
}


void CopyDirectory(const char* dst_path, const char* src_path)
{
	int dfd;
	u64 read;
	Lv2FsDirent dir;
	if (lv2FsOpenDir(src_path, &dfd))
		return;

    mkdir(dst_path, 0777);
    
    read = sizeof(Lv2FsDirent);
	while (!lv2FsReadDir(dfd, &dir, &read)) {
		char newsrc_path[0x440];
        char newdst_path[0x440];
		if (!read)
			break;
		if (dir.d_name[0]== '.')
			continue;

		strcpy(newsrc_path, src_path);
		strcat(newsrc_path, "/");
		strcat(newsrc_path, dir.d_name);

        strcpy(newdst_path, dst_path);
		strcat(newdst_path, "/");
		strcat(newdst_path, dir.d_name);

		if (dir.d_type & DT_DIR) {
			CopyDirectory(newdst_path, newsrc_path);
		} else {
			FILE *fpr, *fpw;
            int n = -1,r, f = 0;
            char buffer[4096];
            
            fpr = fopen(newsrc_path, "r");
            if(fpr) {
                fpw = fopen(newdst_path, "w");
                if(fpw) {

                    while(1) {
                        n = fread(buffer, 1, 4096, fpr);
                        if(n <= 0) break;
                        r = fwrite(buffer, 1, n, fpw);
                        if(r != n) {n = -1; break;}
                    }
                    
                    if(n == 0) f = 1;
                    
                    fclose(fpw);
                }
            fclose(fpr);
            }
            
            if(f == 0) {
                sprintf(msg_error, "Error copying file: %s", dir.d_name);
                sleep(2);
                msg_error[0] = 0;
                usleep(60);
            }

		}
	}
	lv2FsCloseDir(dfd);
}




// thread

int running = 1;

volatile int my_socket=-1;
volatile int flag_exit=0;

struct {
    
    int text;
    int device;
    char name[MAXPATHLEN+1];
    char title[40+1];

} directories[256];

int menu_level    = 0;
int pendrive_test = 0;
int hdd_test      = 0;
int device_mode     = 0;
int ndirectories  = 0;
int curdir = 0;

volatile int hdd_install = 0;

char filename [1024];
char filename2[1024];

u32 color_two = 0xffffffff;

static void control_thread(u64 arg)
{
	
	int i;
    float x=0, y=0;
    static u32 C1, C2, C3, C4, count_frame = 0;
    
    int yesno =0;
	
	while(running ){
       
        ps3pad_read();

        if((new_pad & BUTTON_TRIANGLE) && !menu_level){
			
            menu_level = 2; yesno = 0;
		}

        if((new_pad & BUTTON_SQUARE) && !menu_level  && ndirectories>0){
			
            menu_level = 3; yesno = 0;	
		}

        if((new_pad & BUTTON_CIRCLE) && !menu_level && ndirectories>0 && device_mode){
			
            menu_level = 4; yesno = 0;	
		}

        if(ndirectories <= 0 && (menu_level==1)) menu_level = 0;

        if((new_pad & BUTTON_CROSS)){

                switch(menu_level)
                {
                case 0:
                    
                    if(ndirectories>0) {menu_level = 1; yesno = 0;}

                break;
                
                // launch from device
                case 1:

                    if(yesno) {

                        flag_exit = 2;

                        if(directories[curdir].device)
                            sprintf(bootpath, "/dev_usb/homebrew/%s/EBOOT.BIN", &directories[curdir].name[0]);
                        else
                            sprintf(bootpath, "%s/%s/EBOOT.BIN", ps3load_path, &directories[curdir].name[0]);
                    
                        if(my_socket!=-1)
                            close(my_socket);
                        my_socket=-1;

                    } else menu_level = 0;

                break;
                
                // exit
                case 2:

                    if(yesno) {
                        flag_exit = 1;
                        
                        if(my_socket != -1) {
                    
                            close(my_socket);
                            my_socket=-1;
                        
                        }

                    } else menu_level = 0;

                break;

                // delete from device
                case 3:

                    menu_level = 0;
                    if(yesno) {
                       
                        if(directories[curdir].device)
                            sprintf(filename, "/dev_usb/homebrew/%s", &directories[curdir].name[0]);
                        else
                            sprintf(filename, "%s/%s", ps3load_path, &directories[curdir].name[0]);

                        yesno =0;
            
                        DeleteDirectory(filename);
                        rmdir(filename);
                        ndirectories = 0; curdir = 0; pendrive_test = 0; hdd_test = 0;
                    }

                break;

                // copy from device
                case 4:

                    menu_level = 0;
                    if(yesno) {
                       
                        if(directories[curdir].device) {
                            sprintf(filename, "/dev_usb/homebrew/%s", &directories[curdir].name[0]);
                            sprintf(filename2, "%s/%s", ps3load_path, &directories[curdir].name[0]);
                        } else {
                            sprintf(filename, "%s/%s", ps3load_path, &directories[curdir].name[0]);
                            sprintf(filename2, "/dev_usb/homebrew/%s", &directories[curdir].name[0]);
                        }

                        yesno =0;
            
                        CopyDirectory(filename2, filename);
                        
                        ndirectories = 0; curdir = 0; pendrive_test = 0; hdd_test = 0;
                    }

                break;

                // HDD install 
                case 5:

                    if(yesno) {
                       
                       yesno =0;
                       hdd_install = 1;
                       menu_level  = 0;

                    } else {menu_level = 0; hdd_install = 0;}

                break;

                // USB install 
                case 6:

                    if(yesno) {
                       
                       yesno =0;
                       hdd_install = 2;
                       menu_level  = 0;

                    } else {menu_level = 0; hdd_install = 0;}

                break;

                }
	
		    }

         if(ndirectories<=0 && (menu_level== 2 || menu_level== 5 || menu_level== 6)) {
            
            if((new_pad & BUTTON_LEFT))  yesno = 1;
            if((new_pad & BUTTON_RIGHT)) yesno = 0;

         }

        if(ndirectories > 0) {
            static int b_left = 0, b_right = 0;

         
            if(menu_level) {b_left = b_right = 0;}

            if((old_pad & BUTTON_LEFT) && b_left) {
                b_left++;
                if(b_left > 15) {b_left = 1; new_pad |= BUTTON_LEFT;}
            } else b_left = 0;

            if((old_pad & BUTTON_RIGHT) && b_right) {
                b_right++;
                if(b_right > 15) {b_right = 1; new_pad |= BUTTON_RIGHT;}
            } else b_right = 0;

            if(menu_level) {
                if((new_pad & BUTTON_LEFT))  yesno = 1;
                if((new_pad & BUTTON_RIGHT)) yesno = 0;
            } else {
                if((new_pad & BUTTON_LEFT)) {

                    int next_dir;
                    b_left = 1;
                    next_dir = ((u32)(ndirectories + curdir + 1)) % ndirectories;
                    
                    directories[next_dir].text = -1;

                    next_dir = ((u32)(ndirectories + curdir - 1)) % ndirectories;
                        
                    directories[curdir].text = -1;
                    directories[next_dir].text = -1;
                    
                    curdir = next_dir;
                    next_dir = ((u32)(ndirectories + curdir - 1)) % ndirectories;
                    
                    directories[next_dir].text = -1;

                }

                if((new_pad & BUTTON_RIGHT)) {

                    int next_dir;
                    b_right = 1;
                    next_dir = ((u32)(ndirectories + curdir - 1)) % ndirectories;
                    
                    directories[next_dir].text = -1;

                    next_dir = ((u32)(ndirectories + curdir + 1)) % ndirectories;
                        
                    directories[curdir].text = -1;
                    directories[next_dir].text = -1;
                    
                    curdir = next_dir;
                    next_dir = ((u32)(ndirectories + curdir + 1)) % ndirectories;
                    
                    directories[next_dir].text = -1;

                }
            
            }
        }


        tiny3d_Clear(0xff000000, TINY3D_CLEAR_ALL);

        // Enable alpha Test
        tiny3d_AlphaTest(1, 0x10, TINY3D_ALPHA_FUNC_GEQUAL);

        // Enable alpha blending.
        tiny3d_BlendFunc(1, TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
            NV30_3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA | NV30_3D_BLEND_FUNC_DST_ALPHA_ZERO,
            TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);

        
        C1=RGBA(0, 0x30, 0x80 + (count_frame & 0x7f), 0xff);  
        C2=RGBA(0x80 + (count_frame & 0x7f), 0, 0xff, 0xff);
        C3=RGBA(0,  (count_frame & 0x7f)/4, 0xff - (count_frame & 0x7f), 0xff);
        C4=RGBA(0, 0x80 - (count_frame & 0x7f), 0xff, 0xff);
        
        
        if(count_frame == 127) count_frame = 255;
        if(count_frame == 128) count_frame = 0;
        
        if(count_frame & 128) count_frame--;
        else count_frame++;

        tiny3d_SetPolygon(TINY3D_QUADS);

        tiny3d_VertexPos(0  , 0  , 65535);
        tiny3d_VertexColor(C1);

        tiny3d_VertexPos(847, 0  , 65535);
        tiny3d_VertexColor(C2);

        tiny3d_VertexPos(847, 511, 65535);
        tiny3d_VertexColor(C3);

        tiny3d_VertexPos(0  , 511, 65535);
        tiny3d_VertexColor(C4);


        tiny3d_End();

        update_twat();
    
        if(jpg1.bmp_out) {

            float x2=  ((float) ( (int)(count_frame & 127)- 63)) / 42.666f;
			
			// calculate coordinates for JPG

            x=(848-jpg1.width*2)/2; y=(512-jpg1.height*2)/2;

            tiny3d_SetTexture(0, jpg1_offset, jpg1.width, jpg1.height, jpg1.wpitch, TINY3D_TEX_FORMAT_A8R8G8B8, 1);

            tiny3d_SetPolygon(TINY3D_QUADS);
            
            
            tiny3d_VertexPos(x + x2            , y +x2             , 65534);
            tiny3d_VertexColor(0xffffff10);
            tiny3d_VertexTexture(0.0f , 0.0f);

            tiny3d_VertexPos(x - x2 + jpg1.width*2, y +x2              , 65534);
            tiny3d_VertexTexture(0.99f, 0.0f);

            tiny3d_VertexPos(x + x2 + jpg1.width*2, y -x2+ jpg1.height*2, 65534);
            tiny3d_VertexTexture(0.99f, 0.99f);

            tiny3d_VertexPos(x - x2           , y -x2+ jpg1.height*2, 65534);
            tiny3d_VertexTexture(0.0f , 0.99f);

            tiny3d_End();
		
		}
 
         
        for(i = 0; i< 3; i++) {
            int index;
            
            float w, h, z = (i==1) ? 1 : 2;
            float scale = (i==1) ? 128 : 96;
            
            x=0; y=0;
            
            
            if(ndirectories > 0) index = ((u32) (ndirectories + curdir - 1 + i)) % ndirectories; else index = 0;


        // draw PNG
        
            if(ndirectories > 0 && directories[index].text >= 0 && Png_datas[directories[index].text].bmp_out) {
            
                x=(848 - scale * Png_datas[directories[index].text].width / Png_datas[directories[index].text].height) / 2 - 256 + 256 * i;
                y=(512- scale)/2;

                w = scale * Png_datas[directories[index].text].width / Png_datas[directories[index].text].height;
                h = scale;


                tiny3d_SetTexture(0, Png_offset[directories[index].text], Png_datas[directories[index].text].width, 
                    Png_datas[directories[index].text].height, Png_datas[directories[index].text].wpitch, TINY3D_TEX_FORMAT_A8R8G8B8, 1);

                
               
                if(directories[index].text == 4)
                    DrawTextBox(x, y, z, w, h, 0xffffff60);
                else
                    DrawTextBox(x, y, z, w, h, 0xffffffff);
            
            } else {

                x=(848 - scale * 2/1) / 2 - 256 + 256 * i;
                y=(512- scale)/2;

                w = scale * 2/1;
                h = scale;

                DrawBox(x, y, z, w, h, 0x80008060);
            
            }
        }
        

        SetFontSize(16, 32);
        x=0; y=0;
        
        SetFontColor(0xFFFF00FF, 0x00000000);
        SetFontAutoCenter(1);
        
        if(!v_release) DrawString(x, y, "PS3LoadX " VERSION " - PSL1GHT TEST"); 
        else DrawString(x, y, "PS3LoadX " VERSION " - PSL1GHT");
        
        SetFontAutoCenter(0);

        SetFontSize(12, 24);

        SetFontColor(0xFFFFFFFF, 0x00000000);

        y += 24 * 4;
        
        SetFontSize(24, 48);
        SetFontAutoCenter(1);
        
        if(device_mode) DrawString(0, y, "USB Applications"); else DrawString(0, y, "HDD Applications");

        SetFontAutoCenter(0);

        SetFontSize(16, 32);

        if(ndirectories > 0) {

            SetFontAutoCenter(1);
            x= 0.0; y = 336;
            SetFontColor(0xFFFFFFFF, 0x80008050);

            if(directories[curdir].title[0] != 0)
                DrawFormatString(x, y, "%s", &directories[curdir].title[0]);
            else
                DrawFormatString(x, y, "%s", &directories[curdir].name[0]);

            SetFontAutoCenter(0);
        }

        
        SetFontSize(12, 24);
        SetFontAutoCenter(1);
        x= 0.0; y = 512 - 48;
        if(msg_error[0]!=0) {
            SetFontColor(0xFF3F00FF, 0x00000050);
            DrawFormatString(x, y, "%s", msg_error);
        }
        else {
            SetFontColor(color_two, 0x00000050);
            DrawFormatString(x, y, "%s", msg_two);
        }

        SetFontAutoCenter(0);

        SetFontColor(0xFFFFFFFF, 0x00000000);

        if(menu_level) {

            x= (848-640) / 2; y=(512-360)/2;

           // DrawBox(x, y, 0.0f, 640.0f, 360, 0x006fafe8);
            DrawBox(x - 16, y - 16, 0.0f, 640.0f + 32, 360 + 32, 0x00000038);
            DrawBox(x, y, 0.0f, 640.0f, 360, 0x300030d8);

            SetFontSize(16, 32);
            SetFontAutoCenter(1);
            
            y += 32;
            
            switch(menu_level) {
            
            case 1:
                DrawString(0, y, "Launch Application?");
            break;
            
            case 2:
                DrawString(0, y, "Exit to PS3 Menu?");
            break;

            case 3:
                DrawString(0, y, "Delete Application?");
            break;

            case 4:
                if(device_mode)
                    DrawString(0, y, "Copy from USB to HDD?");
                else
                    DrawString(0, y, "Copy from HDD to USB?");
            break;

            case 5:
                DrawString(0, y, "Install ZIP to HDD?");
            break;
            
            case 6:
                DrawString(0, y, "Install ZIP to USB?");
            break;

            }

            SetFontAutoCenter(0);
            
            y += 100;

            x = 300;

            DrawBox(x, y, 0.0f, 5*16, 32*3, 0x5f00afff);
            
            if(yesno) SetFontColor(0xFFFFFFFF, 0x00000000); else SetFontColor(0x606060FF, 0x00000000);
            
            DrawString(x+16, y+32, "YES");

            x = 848 - 300- 5*16;

            DrawBox(x, y, 0.0f, 5*16, 32*3, 0x5f00afff);
            if(!yesno) SetFontColor(0xFFFFFFFF, 0x00000000); else SetFontColor(0x606060FF, 0x00000000);
            
            DrawString(x+24, y+32, "NO");
		
        }

		sys_ppu_thread_yield();
		
		tiny3d_Flip();
		sysCheckCallback();
		
		
	}
	//you must call this, kthx
	sys_ppu_thread_exit(0);
}

static void file_thread(u64 arg)
{
    int i;
    
    int counter2=0;
    int n;

    Lv2FsFile dir;
    FILE *fp;

    while(running ){

        if((counter2 & 31)==0) {
            int refresh = 0;

            if(lv2FsOpenDir("/dev_usb/homebrew/", &dir) == 0) {
                if(!pendrive_test) {hdd_test = 0; device_mode = 1; refresh = 1;} else lv2FsCloseDir(dir);
            } else {

                if(device_mode == 0 && lv2FsOpenDir("/dev_usb/", &dir) == 0) {
                    mkdir("/dev_usb/homebrew", 0777);
                   lv2FsCloseDir(dir);
                   hdd_test = 0; pendrive_test = 0;
                   ndirectories = 0;
                   curdir = 0;
                   device_mode = 1;
                   continue;
                }

                device_mode = 0;
                pendrive_test = 0;
                if(lv2FsOpenDir(ps3load_path, &dir) == 0) {
                    if(!hdd_test) {device_mode = 0; refresh = 1;} else lv2FsCloseDir(dir);
                } else {
                    ndirectories = 0;
                    curdir = 0;
                }
            }
                

            if(refresh) {
                ndirectories = 0; curdir = 0;
                n = 0;

                while(1) {
                    u64 read = sizeof(Lv2FsDirent);
                    Lv2FsDirent entry;
                    directories[n].text = -1;

                    if(lv2FsReadDir(dir, &entry, &read)<0 || read <= 0) break;

                    if((entry.d_type & 1) && entry.d_name[0] != '.') {
                        strcpy(&directories[n].name[0], entry.d_name);
                        directories[n].title[0] = 0;

                        if(device_mode)
                            sprintf(filename, "/dev_usb/homebrew/%s/title.txt", &directories[n].name[0]);
                        else
                            sprintf(filename, "%s/%s/title.txt", ps3load_path, &directories[n].name[0]);
                       

                        fp =fopen(filename, "r");
                        if(fp){
                            if(!fgets(&directories[n].title[0], 40, fp)) directories[n].title[0] = 0;
                            fclose(fp);
                        }
                        i=0; 
                        while(directories[n].title[i] && i < 40) {
                            if(directories[n].title[i] == 13 || directories[n].title[i] == 10)  break;
                            if(directories[n].title[i] < 32) directories[n].title[i] = 32; i++;}
                        directories[n].device = device_mode;
                        n++;
                    }
                    
                }
                
                

                ndirectories = n;
            }
            if(device_mode) pendrive_test = 1; else hdd_test = 1;

            lv2FsCloseDir(dir);
        } 


        if(ndirectories > 0) {
            for(i = 0; i< 3; i++) {
                int index = ((u32) (ndirectories + curdir - 1 + i)) % ndirectories;

                // LOAD PNG
                
                if(ndirectories && directories[index].text<0) {
                    
                    if(directories[index].device)
                        sprintf(filename, "/dev_usb/homebrew/%s/ICON0.PNG", &directories[index].name[0]);
                    else
                        sprintf(filename, "%s/%s/ICON0.PNG", ps3load_path, &directories[index].name[0]);

                    if(LoadTexturePNG(filename, i) == 0) directories[index].text = i;
                    else {
                        directories[index].text = 4;
                        memcpy(&Png_datas[4], &jpg1, sizeof(JpgDatas));
                        Png_offset[4] = jpg1_offset;
                    }
                }
            }
        }


        counter2++;

        usleep(20000);

	}
	//you must call this, kthx
	sys_ppu_thread_exit(0);
}



static void sys_callback(uint64_t status, uint64_t param, void* userdata) {

     switch (status) {
		case EVENT_REQUEST_EXITAPP:
			if(my_socket!=-1) {
				flag_exit=1;
				close(my_socket);
				my_socket=-1;
			}
			release_all();
			sysProcessExit(1);
			break;
      
       default:
		   break;
         
	}
}

sys_ppu_thread_t thread1_id, thread2_id;

void release_all() {

	u64 retval;

	sysUnregisterCallback(EVENT_SLOT0);
	running= 0;
	sys_ppu_thread_join(thread1_id, &retval);
    sys_ppu_thread_join(thread2_id, &retval);

	SysUnloadModule(SYSMODULE_JPGDEC);
    SysUnloadModule(SYSMODULE_PNGDEC);
    SysUnloadModule(SYSMODULE_FS);

}


int main(int argc, const char* argv[], const char* envp[])
{
	SysLoadModule(SYSMODULE_FS);
    SysLoadModule(SYSMODULE_PNGDEC);
	SysLoadModule(SYSMODULE_JPGDEC);

    if(argc>0 && argv) {
    
        if(!strncmp(argv[0], "/dev_hdd0/game/", 15)) {
            int n;

            strcpy(ps3load_path, argv[0]);

            n= 15; while(ps3load_path[n] != '/' && ps3load_path[n] != 0) n++;

            if(ps3load_path[n] == '/') {
                sprintf(&ps3load_path[n], "%s", "/RELOAD.SELF");
                lv2FsChmod(ps3load_path, 0170777ULL);

                sprintf(&ps3load_path[n], "%s", "/homebrew");
                v_release = 1;
            }
        }
    }

	msg_error[0] = 0; // clear msg_error
    //msg_one  [0] = 0;
    msg_two  [0] = 0;

    tiny3d_Init(1024*1024);
    
    LoadTexture();
    init_twat();

    my_socket = -1;

    ERROR(netInitialize(), "Error initializing network");

    mkdir(ZIP_PATH, 0777);
	DeleteDirectory(ZIP_PATH);
    mkdir(ps3load_path, 0777);

	ioPadInit(7);
	

    sys_ppu_thread_create( &thread1_id, control_thread, 0ULL, 999, 256*1024, THREAD_JOINABLE, "Control Thread ps3load");
    sys_ppu_thread_create( &thread2_id, file_thread, 0ULL, 1000, 256*1024, THREAD_JOINABLE, "File Thread ps3load");

	// register exit callback
	sysRegisterCallback(EVENT_SLOT0, sys_callback, NULL);

    sprintf(msg_two, "Creating socket...");

	my_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	ERROR(my_socket, "Error creating socket()");

	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(PORT);
    
    if(my_socket!=-1) {
        sprintf(msg_two, "Binding socket...");

        ERROR(bind(my_socket, (struct sockaddr*)&server, sizeof(server)), "Error bind()ing socket");
    }

    if(my_socket!=-1)
	    ERROR(listen(my_socket, 10), "Error calling listen()");

#define continueloop() { close(c); goto reloop; }

reloop:

    msg_two[0]   = 0;

	while (1) {
		
        usleep(20000);
		
        if(flag_exit) break;

        if(my_socket == -1) continue;

        color_two = 0xffffffff;

		sprintf(msg_two, "Waiting for connection...");
		
		int c = accept(my_socket, NULL, NULL);

		if(flag_exit) break;
        
        if(my_socket == -1) continue;

		ERROR(c, "Error calling accept()");

		u32 magic = 0;
		if (read(c, &magic, sizeof(magic)) < 0)
			continueloop();
		if (strncmp((char*)&magic, "HAXX", 4)) {
			sprintf(msg_two, "Wrong HAXX magic.");
			continueloop();
		}
		if (read(c, &magic, sizeof(magic)) < 0)
			continueloop();
		u16 argslen = magic & 0xFFFF;
		
		u32 filesize = 0;
		if (read(c, &filesize, sizeof(filesize)) < 0)
			continueloop();

		u32 uncompressed = 0;
		if (read(c, &uncompressed, sizeof(uncompressed)) < 0)
			continueloop();

        color_two = 0x00ff00ff;

		sprintf(msg_two, "Receiving data... (0x%08x/0x%08x)", filesize, uncompressed);

		u8* data = (u8*)malloc(filesize);
		u32 pos = 0;
		u32 count;
		while (pos < filesize) {
			u32 count = MIN(0x1000, filesize - pos);
			int ret = read(c, data + pos, count);
			if (ret < 0)
				continueloop();
			pos += ret;
		}

		sprintf(msg_two, "Receiving arguments... 0x%08x", argslen);

		u8* args = NULL;
		if (argslen) {
			args = (u8*)malloc(argslen);
			if (read(c, args, argslen) < 0)
				continueloop();
		}

		close(c);

		sprintf(msg_two, "Decompressing...");

		if (uncompressed) {
			u8* compressed = data;
			uLongf final = uncompressed;
			data = (u8*)malloc(final);
			int ret = uncompress(data, &final, compressed, filesize);
			if (ret != Z_OK)
				continue;
			free(compressed);
			if (uncompressed != final)
				continue;
			uncompressed = final;
		} else
			uncompressed = filesize;

		sprintf(msg_two, "Launching...");

		int fd = open(SELF_PATH, O_CREAT | O_TRUNC | O_WRONLY);
		ERROR2(fd, "Error opening temporary file.");

		pos = 0;
		while (pos < uncompressed) {
			count = MIN(0x1000, uncompressed - pos);
			write(fd, data + pos, count);
			pos += count;
		}

		close(fd);

        free(data);

		
		strcpy(bootpath, SELF_PATH);

		struct zip* archive = zip_open(SELF_PATH, ZIP_CHECKCONS, NULL);
		int files = zip_get_num_files(archive);
		if (files > 0) {
            
            sprintf(msg_two, "Installing ZIP ...");
			strcpy(bootpath, "");

			for (int i = 0; i < files; i++) {
				char path[MAXPATHLEN];
				
				const char* filename = zip_get_name(archive, i, 0);
				if (!filename)
					continue;
				
     
                if(i==0) {
                    if (filename[strlen(filename) - 1] == '/') {
                        strcpy(install_folder, filename);

                        if(menu_level) {
                            menu_level = 0;
                            usleep(200000);
                        }

                       menu_level = 5 + device_mode;
                       hdd_install = -1;

                       while(1) {
                           if(hdd_install != -1) break;
                           usleep(50000);
                       }

                       if(hdd_install == 0) {
                           zip_close(archive);
                           ERROR2(-1, "ZIP install cancelled");
                       }

                       if(hdd_install == 2) mkdir("/dev_usb/homebrew", 0777);


                    } else {
                        zip_close(archive);
                        ERROR2(-1, "ZIP files must starts with install folder");
                    }
                } else {
                    if (strncmp(filename, install_folder, strlen(install_folder))) {
                        zip_close(archive);
                        ERROR2(-1, "Only one folder supported in root of ZIP files");
                    }
                }
                
                if(hdd_install==1)
                    strcpy(path, ps3load_path);
                else
                    strcpy(path, "/dev_usb/homebrew");

                if (filename[0] != '/')
					strcat(path, "/");
				strcat(path, filename);

				#define ENDS_WITH(needle) \
					(strlen(filename) >= strlen(needle) && !strcasecmp(filename + strlen(filename) - strlen(needle), needle))

				/*if (ENDS_WITH("EBOOT.BIN") || ENDS_WITH(".self"))
					strcpy(bootpath, path);*/

				if (filename[strlen(filename) - 1] != '/') {
					struct zip_stat st;
					if (zip_stat_index(archive, i, 0, &st)) {
						sprintf(msg_two, "Unable to access file %s in zip.", filename);
						continue;
					}
					struct zip_file* zfd = zip_fopen_index(archive, i, 0);
					if (!zfd) {
						sprintf(msg_two, "Unable to open file %s in zip.", filename);
						continue;
					}

					int tfd = open(path, O_CREAT | O_TRUNC | O_WRONLY);
                    if(tfd < 0) {
					    zip_fclose(zfd);
                        zip_close(archive);
                        ERROR2(tfd, "Error opening temporary file.");
                    }

					pos = 0;
					u8* buffer = malloc(0x1000);
					while (pos < st.size) {
						count = MIN(0x1000, st.size - pos);
						if (zip_fread(zfd, buffer, count) != count) {

                            free(buffer);

                            zip_fclose(zfd);
                            close(tfd);
                            zip_close(archive);
							
                            ERROR2(-1, "Error reading from zip.");
                            
                        }

						write(tfd, buffer, count);
						pos += count;
					}
					free(buffer);

					zip_fclose(zfd);
					close(tfd);
				} else
					mkdir(path, 0777);
			}
		}
		if (archive) {
            zip_close(archive);
            ndirectories = 0; curdir = 0; pendrive_test = 0; hdd_test=0;
        }

		if (!strlen(bootpath))
			continue;

        int bfd = open(bootpath, O_RDONLY);
        
        if(bfd >= 0) close(bfd); else {ERROR2(-1, "SELF cannot be opened");}



		char* launchenvp[2];
		char* launchargv[MAX_ARG_COUNT];
		memset(launchenvp, 0, sizeof(launchenvp));
		memset(launchargv, 0, sizeof(launchargv));

		launchenvp[0] = (char*)malloc(0x440);
		snprintf(launchenvp[0], 0x440, "ps3load=%s", argv[0]);

		pos = 0;
		int i = 0;
		while (pos < argslen) {
			int len = strlen((char*)args + pos);
			if (!len)
				break;
			launchargv[i] = (char*)malloc(len + 1);
			strcpy(launchargv[i], (char*)args + pos);
			pos += len + 1;
			i++;
		}

		release_all();
		sleep(1);
		sysProcessExitSpawn2(bootpath, (const char**)launchargv, (const char**)launchenvp, NULL, 0, 1001, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
	}

    if(flag_exit == 2) {
        release_all();
		sleep(1);
        sysProcessExitSpawn2(bootpath, NULL, NULL, NULL, 0, 1001, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
    }
	sprintf(msg_two, "Exiting...");
    usleep(250);
	release_all();

	sleep(2);
	return 0;
}

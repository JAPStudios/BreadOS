#include "desktop.h"
#include "video.h"
#include "theme.h"
#include "memory.h"
#include "task.h"
#include "kernel/keyboard.h"
#include "kernel/mouse.h"
#include <stddef.h>
#include <stdint.h>

#define DESKTOP_WORKSPACES 4u
#define DESKTOP_WINDOWS 12u
#define APP_SHELL 0u
#define APP_FILES 1u
#define APP_APPEARANCE 2u
#define APP_SYSTEM 3u

struct desktop_window {
    uint32_t x,y,width,height;
    uint8_t app,workspace,open;
};

extern uint32_t timer_ticks(void);
extern size_t bread_file_count(void);
extern const char *bread_file_name(size_t index);

static struct desktop_window windows[DESKTOP_WINDOWS];
static uint32_t active_workspace,pointer_x,pointer_y,focused_window;
static int dragging=-1;
static int32_t drag_x,drag_y;
static uint8_t previous_buttons;
static uint32_t screen_width,screen_height;

static const char *app_title(uint8_t app) {
    switch(app) {
        case APP_FILES:return "Files";
        case APP_APPEARANCE:return "Appearance";
        case APP_SYSTEM:return "System";
        default:return "Shell";
    }
}

static void text(uint32_t x,uint32_t y,const char *value,uint32_t color,
                 uint32_t background) {
    video_draw_text(x,y,value,color,background);
}

static void decimal(uint32_t value,char output[11]) {
    char reversed[10];uint32_t count=0;
    do {reversed[count++]=(char)('0'+value%10u);value/=10u;} while(value && count<10u);
    for(uint32_t i=0;i<count;i++)output[i]=reversed[count-1u-i];
    output[count]=0;
}

static void draw_window(uint32_t index) {
    const struct desktop_window *w=&windows[index];
    if (!w->open || w->workspace!=active_workspace) return;
    const struct bread_theme *theme=theme_current();
    uint32_t border=index==focused_window?theme->accent:0x596273u;
    uint32_t title_bg=index==focused_window?theme->accent:0x303846u;
    video_fill_rect(w->x,w->y,w->width,w->height,border);
    video_fill_rect(w->x+2u,w->y+2u,w->width-4u,w->height-4u,theme->background);
    video_fill_rect(w->x+2u,w->y+2u,w->width-4u,22u,title_bg);
    text(w->x+10u,w->y+5u,app_title(w->app),theme->foreground,title_bg);
    video_fill_rect(w->x+w->width-22u,w->y+5u,14u,14u,0xB94A48u);
    text(w->x+w->width-19u,w->y+4u,"X",0xFFFFFFu,0xB94A48u);

    uint32_t tx=w->x+12u,ty=w->y+34u;
    if (w->app==APP_SYSTEM) {
        text(tx,ty,"BREADOS SYSTEM STATUS",theme->accent,theme->background);ty+=24u;
        text(tx,ty,"Kernel: BreadOS native x86",theme->foreground,theme->background);ty+=20u;
        char value[11];
        text(tx,ty,"Free 4 KiB frames:",theme->foreground,theme->background);
        decimal(memory_free_frames(),value);text(tx+160u,ty,value,theme->accent,theme->background);ty+=20u;
        text(tx,ty,"Task switches:",theme->foreground,theme->background);
        decimal(task_switch_count(),value);text(tx+160u,ty,value,theme->accent,theme->background);ty+=20u;
        text(tx,ty,"PIT ticks:",theme->foreground,theme->background);
        decimal(timer_ticks(),value);text(tx+160u,ty,value,theme->accent,theme->background);
    } else if (w->app==APP_FILES) {
        text(tx,ty,"BREADFS FILES",theme->accent,theme->background);ty+=24u;
        size_t count=bread_file_count();
        if (!count) text(tx,ty,"No files on this volume.",theme->foreground,theme->background);
        else {
            size_t limit=count<10u?count:10u;
            for(size_t i=0;i<limit && ty+16u<w->y+w->height-8u;i++) {
                const char *name=bread_file_name(i);
                if(name) text(tx,ty,name,theme->foreground,theme->background);
                ty+=18u;
            }
            if(count>limit) text(tx,ty,"More files exist; use ls in Shell.",theme->foreground,theme->background);
        }
    } else if (w->app==APP_APPEARANCE) {
        text(tx,ty,"SELECT A COLOR THEME",theme->accent,theme->background);
        uint32_t list_y=ty+24u;
        for(size_t i=0;i<theme_count();i++) {
            uint32_t column=(uint32_t)(i/12u),row=(uint32_t)(i%12u);
            uint32_t px=tx+column*152u,py=list_y+row*18u;
            if(py+16u>w->y+w->height-4u) continue;
            const struct bread_theme *entry=theme_at(i);
            uint32_t bg=entry==theme_current()?entry->accent:theme->background;
            uint32_t fg=entry==theme_current()?entry->background:theme->foreground;
            text(px,py,entry->name,fg,bg);
        }
    } else {
        text(tx,ty,"BREADOS DESKTOP SHELL",theme->accent,theme->background);ty+=24u;
        text(tx,ty,"Keyboard shortcuts:",theme->foreground,theme->background);ty+=20u;
        text(tx,ty,"T  Appearance   F  Files",theme->foreground,theme->background);ty+=18u;
        text(tx,ty,"S  System       E  Shell",theme->foreground,theme->background);ty+=18u;
        text(tx,ty,"[ / ]  Switch workspace",theme->foreground,theme->background);ty+=18u;
        text(tx,ty,"Q  Return to kernel console",theme->foreground,theme->background);
    }
}

static void render(void) {
    const struct bread_theme *theme=theme_current();
    video_clear(theme->background);
    video_fill_rect(0,0,screen_width,28u,0x171C25u);
    text(12u,6u,"BREADOS",0xFFFFFFu,0x171C25u);
    text(92u,6u,"NATIVE DESKTOP",theme->accent,0x171C25u);
    for(uint32_t i=0;i<DESKTOP_WORKSPACES;i++) {
        uint32_t x=screen_width-116u+i*26u;
        uint32_t bg=i==active_workspace?theme->accent:0x343D49u;
        video_fill_rect(x,4u,22u,20u,bg);
        char digit[2]={(char)('1'+i),0};
        text(x+7u,6u,digit,i==active_workspace?theme->background:0xFFFFFFu,bg);
    }
    for(uint32_t i=0;i<DESKTOP_WINDOWS;i++)
        if(i!=focused_window) draw_window(i);
    if(focused_window<DESKTOP_WINDOWS) draw_window(focused_window);
    video_fill_rect(0,screen_height-42u,screen_width,42u,0x171C25u);
    const char *labels[4]={"SHELL","FILES","THEMES","SYSTEM"};
    for(uint32_t i=0;i<4;i++) {
        uint32_t x=screen_width/2u-112u+i*58u;
        video_fill_rect(x,screen_height-36u,52u,30u,i==0?theme->accent:0x343D49u);
        text(x+5u,screen_height-28u,labels[i],0xFFFFFFu,i==0?theme->accent:0x343D49u);
    }
    video_fill_rect(pointer_x,pointer_y,2u,13u,0xFFFFFFu);
    video_fill_rect(pointer_x,pointer_y,9u,2u,0xFFFFFFu);
}

static int open_app(uint8_t app) {
    for(uint32_t i=0;i<DESKTOP_WINDOWS;i++) {
        if(windows[i].open && windows[i].workspace==active_workspace && windows[i].app==app) {
            focused_window=i;
            return 0;
        }
    }
    for(uint32_t i=0;i<DESKTOP_WINDOWS;i++) {
        if(!windows[i].open) {
            uint32_t width=screen_width>420u?400u:screen_width-20u;
            uint32_t height=screen_height>340u?300u:screen_height-90u;
            if(width<180u || height<100u) return -1;
            windows[i].x=24u+(i%5u)*22u;
            windows[i].y=38u+(i%5u)*18u;
            if(windows[i].x+width>screen_width) windows[i].x=10u;
            if(windows[i].y+height>screen_height-46u) windows[i].y=34u;
            windows[i].width=width;
            windows[i].height=height;
            windows[i].app=app;
            windows[i].workspace=(uint8_t)active_workspace;
            windows[i].open=1;
            focused_window=i;
            return 0;
        }
    }
    return -1;
}

static void switch_workspace(uint32_t workspace) {
    if(workspace>=DESKTOP_WORKSPACES) return;
    active_workspace=workspace;
    focused_window=DESKTOP_WINDOWS;
    for(uint32_t i=0;i<DESKTOP_WINDOWS;i++)
        if(windows[i].open && windows[i].workspace==active_workspace) focused_window=i;
}

static void mouse_event(const struct mouse_event *event) {
    pointer_x=event->x;
    pointer_y=event->y;
    uint8_t pressed=(uint8_t)(event->buttons&1u);
    if(pressed && !previous_buttons) {
        if(pointer_y<28u && pointer_x>=screen_width-116u) {
            uint32_t workspace=(pointer_x-(screen_width-116u))/26u;
            if(workspace<DESKTOP_WORKSPACES) switch_workspace(workspace);
        } else if(pointer_y>=screen_height-42u) {
            uint32_t slot=(pointer_x+112u-screen_width/2u)/58u;
            if(pointer_x+112u>=screen_width/2u && slot<4u) open_app((uint8_t)slot);
        } else {
            for(int32_t i=(int32_t)DESKTOP_WINDOWS-1;i>=0;i--) {
                struct desktop_window *w=&windows[i];
                if(!w->open || w->workspace!=active_workspace) continue;
                if(pointer_x<w->x || pointer_x>=w->x+w->width ||
                   pointer_y<w->y || pointer_y>=w->y+w->height) continue;
                focused_window=(uint32_t)i;
                if(pointer_y<w->y+24u) {
                    if(pointer_x>=w->x+w->width-24u) w->open=0;
                    else {dragging=i;drag_x=(int32_t)pointer_x-(int32_t)w->x;drag_y=(int32_t)pointer_y-(int32_t)w->y;}
                } else if(w->app==APP_APPEARANCE && pointer_y>=w->y+58u) {
                    uint32_t column=(pointer_x-(w->x+12u))/152u;
                    uint32_t row=(pointer_y-(w->y+58u))/18u;
                    uint32_t index=column*12u+row;
                    const struct bread_theme *choice=theme_at(index);
                    if(choice) theme_apply(choice->name);
                }
                break;
            }
        }
    }
    if(pressed && dragging>=0) {
        struct desktop_window *w=&windows[(uint32_t)dragging];
        int32_t nx=(int32_t)pointer_x-drag_x,ny=(int32_t)pointer_y-drag_y;
        if(nx<0) nx=0;
        if(ny<28) ny=28;
        if((uint32_t)nx+w->width>screen_width) nx=(int32_t)(screen_width-w->width);
        if((uint32_t)ny+w->height>screen_height-42u) ny=(int32_t)(screen_height-42u-w->height);
        w->x=(uint32_t)nx;w->y=(uint32_t)ny;
    }
    if(!pressed) dragging=-1;
    previous_buttons=pressed;
}

int desktop_run(void) {
    if(!video_available()) return -1;
    screen_width=video_width();screen_height=video_height();
    if(screen_width<240u || screen_height<200u) return -1;
    for(uint32_t i=0;i<DESKTOP_WINDOWS;i++) windows[i].open=0;
    active_workspace=0;focused_window=DESKTOP_WINDOWS;dragging=-1;previous_buttons=0;
    pointer_x=screen_width/2u;pointer_y=screen_height/2u;
    open_app(APP_SHELL);
    open_app(APP_SYSTEM);
    render();
    for(;;) {
        int changed=0;
        struct mouse_event event;
        while(mouse_read_event(&event)){mouse_event(&event);changed=1;}
        char key;
        while(keyboard_read_char(&key)) {
            if(key=='q' || key=='Q') return 0;
            if(key=='t' || key=='T'){open_app(APP_APPEARANCE);changed=1;}
            else if(key=='f' || key=='F'){open_app(APP_FILES);changed=1;}
            else if(key=='s' || key=='S'){open_app(APP_SYSTEM);changed=1;}
            else if(key=='e' || key=='E'){open_app(APP_SHELL);changed=1;}
            else if(key=='[' && active_workspace) {switch_workspace(active_workspace-1u);changed=1;}
            else if(key==']' && active_workspace+1u<DESKTOP_WORKSPACES){switch_workspace(active_workspace+1u);changed=1;}
            else if(key>='1' && key<='4'){switch_workspace((uint32_t)(key-'1'));changed=1;}
        }
        if(changed) render();
        __asm__ volatile("hlt");
    }
}

#include "desktop.h"
#include "video.h"
#include "theme.h"
#include "memory.h"
#include "task.h"
#include "kernel/keyboard.h"
#include "kernel/mouse.h"
#include "notification.h"
#include "clipboard.h"
#include "reminder.h"
#include "idle.h"
#include "launcher.h"
#include <stddef.h>
#include <stdint.h>

#define DESKTOP_WORKSPACES 4u
#define DESKTOP_WINDOWS 12u
#define APP_SHELL 0u
#define APP_FILES 1u
#define APP_APPEARANCE 2u
#define APP_SYSTEM 3u
#define APP_NOTIFICATIONS 4u
#define APP_CLIPBOARD 5u
#define APP_LAUNCHER 6u
#define LAYOUT_DWINDLE 0u
#define LAYOUT_SCROLL 1u
#define LAYOUT_FLOAT 2u

struct desktop_window {
    uint32_t x,y,width,height;
    uint8_t app,workspace,open;
};

extern uint32_t timer_ticks(void);
extern size_t bread_file_count(void);
extern const char *bread_file_name(size_t index);
extern int bread_file_read(const char *name,char *out,size_t capacity);

static struct desktop_window windows[DESKTOP_WINDOWS];
static uint32_t active_workspace,pointer_x,pointer_y,focused_window;
static size_t selected_file;
static char launcher_query[32];
static size_t launcher_results[8],launcher_result_count;
static uint8_t workspace_layout[DESKTOP_WORKSPACES];
static int32_t workspace_scroll[DESKTOP_WORKSPACES];
static int dragging=-1;
static int32_t drag_x,drag_y;
static uint8_t previous_buttons;
static uint32_t screen_width,screen_height;

static const char *app_title(uint8_t app) {
    switch(app) {
        case APP_FILES:return "Files";
        case APP_APPEARANCE:return "Appearance";
        case APP_SYSTEM:return "System";
        case APP_NOTIFICATIONS:return "Notifications";
        case APP_CLIPBOARD:return "Clipboard";
        case APP_LAUNCHER:return "Launcher";
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

static const char *layout_title(void) {
    if(workspace_layout[active_workspace]==LAYOUT_SCROLL) return "SCROLL";
    if(workspace_layout[active_workspace]==LAYOUT_FLOAT) return "FLOAT";
    return "DWINDLE";
}

static void dwindle_rect(uint32_t rank,uint32_t count,uint32_t depth,
                         uint32_t x,uint32_t y,uint32_t width,uint32_t height,
                         uint32_t *out_x,uint32_t *out_y,
                         uint32_t *out_width,uint32_t *out_height) {
    if(count<=1u) {
        *out_x=x;*out_y=y;*out_width=width;*out_height=height;return;
    }
    if((depth&1u)==0u) {
        uint32_t first=width/2u;
        if(rank==0u) {*out_x=x;*out_y=y;*out_width=first;*out_height=height;}
        else dwindle_rect(rank-1u,count-1u,depth+1u,x+first,y,width-first,height,
                          out_x,out_y,out_width,out_height);
    } else {
        uint32_t first=height/2u;
        if(rank==0u) {*out_x=x;*out_y=y;*out_width=width;*out_height=first;}
        else dwindle_rect(rank-1u,count-1u,depth+1u,x,y+first,width,height-first,
                          out_x,out_y,out_width,out_height);
    }
}

static void arranged_window(uint32_t index,struct desktop_window *out) {
    *out=windows[index];
    if(!out->open || out->workspace!=active_workspace ||
       workspace_layout[active_workspace]==LAYOUT_FLOAT) return;
    uint32_t rank=0,count=0;
    for(uint32_t i=0;i<DESKTOP_WINDOWS;i++) {
        if(windows[i].open && windows[i].workspace==active_workspace) {
            if(i==index) rank=count;
            count++;
        }
    }
    if(!count) return;
    uint32_t area_x=8u,area_y=34u,area_w=screen_width-16u,area_h=screen_height-84u;
    if(workspace_layout[active_workspace]==LAYOUT_DWINDLE) {
        dwindle_rect(rank,count,0u,area_x,area_y,area_w,area_h,
                     &out->x,&out->y,&out->width,&out->height);
    } else {
        int32_t x=(int32_t)area_x+(int32_t)(rank*56u)-workspace_scroll[active_workspace];
        out->x=x<0?0u:(uint32_t)x;
        out->y=area_y;
        out->width=area_w>72u?area_w-40u:area_w;
        out->height=area_h;
    }
    if(out->width<120u) out->width=120u;
    if(out->height<80u) out->height=80u;
}

static void draw_window(uint32_t index) {
    struct desktop_window view;
    arranged_window(index,&view);
    const struct desktop_window *w=&view;
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
            size_t limit=count<8u?count:8u;
            for(size_t i=0;i<limit && ty+16u<w->y+w->height-8u;i++) {
                const char *name=bread_file_name(i);
                if(name) text(tx,ty+24u+(uint32_t)i*18u,name,
                              i==selected_file?theme->accent:theme->foreground,theme->background);
            }
            if(selected_file<count) {
                const char *name=bread_file_name(selected_file);
                char content[256];
                if(name) text(tx+160u,ty+24u,name,theme->accent,theme->background);
                if(name && bread_file_read(name,content,sizeof(content))>=0) {
                    size_t pos=0;uint32_t line_y=ty+44u;
                    for(uint32_t row=0;row<10u && content[pos] && line_y<w->y+w->height-8u;row++) {
                        char line[25];size_t length=0;
                        while(content[pos] && content[pos]!='\n' && length<24u)
                            line[length++]=content[pos++];
                        if(content[pos]=='\n') pos++;
                        line[length]=0;
                        text(tx+160u,line_y,line,theme->foreground,theme->background);
                        line_y+=18u;
                    }
                }
            }
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
    } else if(w->app==APP_CLIPBOARD) {
        text(tx,ty,"CURRENT CLIPBOARD",theme->accent,theme->background);ty+=20u;
        text(tx,ty,clipboard_current(),theme->foreground,theme->background);ty+=28u;
        text(tx,ty,"HISTORY - CLICK TO RESTORE",theme->accent,theme->background);ty+=22u;
        size_t count=clipboard_history_count();
        if(!count) text(tx,ty,"No retained entries.",theme->foreground,theme->background);
        for(size_t i=0;i<count && i<6u;i++) {
            const char *entry=clipboard_history_at(i);
            if(entry) text(tx,ty,entry,theme->foreground,theme->background);
            ty+=18u;
        }
    } else if(w->app==APP_LAUNCHER) {
        text(tx,ty,"SEARCH APPLICATIONS",theme->accent,theme->background);ty+=22u;
        text(tx,ty,launcher_query,theme->foreground,theme->background);ty+=24u;
        launcher_result_count=bread_launcher_search(launcher_query,launcher_results,8u);
        if(!launcher_result_count) text(tx,ty,"No matching applications.",theme->foreground,theme->background);
        for(size_t i=0;i<launcher_result_count && i<8u && ty+38u<w->y+w->height-4u;i++) {
            const struct bread_launcher_entry *entry=bread_launcher_entry_at(launcher_results[i]);
            if(!entry) continue;
            text(tx,ty,entry->name,theme->accent,theme->background);
            text(tx+112u,ty,entry->category,theme->foreground,theme->background);ty+=18u;
            text(tx+8u,ty,entry->description,theme->foreground,theme->background);ty+=20u;
        }
    } else if(w->app==APP_NOTIFICATIONS) {
        text(tx,ty,"NOTIFICATION HISTORY",theme->accent,theme->background);ty+=24u;
        size_t count=notification_count();
        if(!count) text(tx,ty,"No notifications recorded.",theme->foreground,theme->background);
        for(size_t i=0;i<count && i<5u;i++) {
            const struct bread_notification *entry=notification_get(i);
            if(!entry) continue;
            text(tx,ty,entry->sender,theme->accent,theme->background);
            text(tx+104u,ty,entry->title,theme->foreground,theme->background);ty+=18u;
            text(tx,ty,entry->body,theme->foreground,theme->background);ty+=24u;
        }
        text(w->x+w->width-190u,w->y+w->height-22u,"Click a row to dismiss",theme->accent,theme->background);
    } else {
        text(tx,ty,"BREADOS DESKTOP SHELL",theme->accent,theme->background);ty+=24u;
        text(tx,ty,"Keyboard shortcuts:",theme->foreground,theme->background);ty+=20u;
        text(tx,ty,"T  Appearance   F  Files",theme->foreground,theme->background);ty+=18u;
        text(tx,ty,"S  System       E  Shell",theme->foreground,theme->background);ty+=18u;
        text(tx,ty,"[ / ]  Switch workspace",theme->foreground,theme->background);ty+=18u;
        text(tx,ty,"L  Cycle layouts",theme->foreground,theme->background);ty+=18u;
        text(tx,ty,", / .  Scroll layout",theme->foreground,theme->background);ty+=18u;
        text(tx,ty,"N  Notifications  C  Clipboard",theme->foreground,theme->background);ty+=18u;
        text(tx,ty,"D  Do Not Disturb",theme->foreground,theme->background);ty+=18u;
        text(tx,ty,"Q  Return to kernel console",theme->foreground,theme->background);
    }
}

static void draw_notification_toasts(void) {
    if(notification_dnd()) return;
    const struct bread_theme *theme=theme_current();
    uint32_t shown=0;
    for(size_t i=0;i<notification_count() && shown<3u;i++) {
        if(!notification_is_visible(i,timer_ticks())) continue;
        const struct bread_notification *entry=notification_get(i);
        if(!entry) continue;
        uint32_t x=screen_width>270u?screen_width-258u:8u;
        uint32_t y=34u+shown*66u;
        video_fill_rect(x,y,250u,58u,theme->accent);
        video_fill_rect(x+2u,y+2u,246u,54u,theme->background);
        text(x+8u,y+5u,entry->sender,theme->accent,theme->background);
        text(x+8u,y+22u,entry->title,theme->foreground,theme->background);
        text(x+8u,y+39u,entry->body,theme->foreground,theme->background);
        shown++;
    }
}

static void render(void) {
    const struct bread_theme *theme=theme_current();
    video_clear(theme->background);
    video_fill_rect(0,0,screen_width,28u,0x171C25u);
    text(12u,6u,"BREADOS",0xFFFFFFu,0x171C25u);
    text(92u,6u,"NATIVE DESKTOP",theme->accent,0x171C25u);
    text(224u,6u,layout_title(),theme->foreground,0x171C25u);
    if(notification_dnd() && screen_width>480u) text(300u,6u,"DND",0xF0C674u,0x171C25u);
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
    draw_notification_toasts();
    video_fill_rect(0,screen_height-42u,screen_width,42u,0x171C25u);
    const char *labels[5]={"SHELL","FILES","THEMES","SYSTEM","ALERTS"};
    for(uint32_t i=0;i<5;i++) {
        uint32_t x=screen_width/2u-145u+i*58u;
        video_fill_rect(x,screen_height-36u,52u,30u,i==0?theme->accent:0x343D49u);
        text(x+5u,screen_height-28u,labels[i],0xFFFFFFu,i==0?theme->accent:0x343D49u);
    }
    video_fill_rect(pointer_x,pointer_y,2u,13u,0xFFFFFFu);
    video_fill_rect(pointer_x,pointer_y,9u,2u,0xFFFFFFu);
}

static int open_app(uint8_t app) {
    if(app==APP_LAUNCHER) launcher_query[0]=0;
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

static void launch_result(size_t result) {
    if(result>=launcher_result_count) return;
    const struct bread_launcher_entry *entry=bread_launcher_entry_at(launcher_results[result]);
    if(!entry) return;
    for(uint32_t i=0;i<DESKTOP_WINDOWS;i++) if(windows[i].open && windows[i].app==APP_LAUNCHER) windows[i].open=0;
    open_app(entry->app_id);
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
            uint32_t slot=(pointer_x+145u-screen_width/2u)/58u;
            if(pointer_x+145u>=screen_width/2u && slot<5u) open_app((uint8_t)slot);
        } else {
            for(int32_t i=(int32_t)DESKTOP_WINDOWS-1;i>=0;i--) {
                struct desktop_window *w=&windows[i];
                struct desktop_window view;
                arranged_window((uint32_t)i,&view);
                if(!w->open || w->workspace!=active_workspace) continue;
                if(pointer_x<view.x || pointer_x>=view.x+view.width ||
                   pointer_y<view.y || pointer_y>=view.y+view.height) continue;
                focused_window=(uint32_t)i;
                if(pointer_y<view.y+24u) {
                    if(pointer_x>=view.x+view.width-24u) w->open=0;
                    else if(workspace_layout[active_workspace]==LAYOUT_FLOAT) {
                        dragging=i;drag_x=(int32_t)pointer_x-(int32_t)view.x;drag_y=(int32_t)pointer_y-(int32_t)view.y;
                    }
                } else if(w->app==APP_FILES && pointer_y>=view.y+58u && pointer_x<view.x+156u) {
                    uint32_t row=(pointer_y-(view.y+58u))/18u;
                    if(row<bread_file_count() && row<8u) selected_file=row;
                } else if(w->app==APP_CLIPBOARD && pointer_y>=view.y+80u) {
                    uint32_t row=(pointer_y-(view.y+80u))/18u;
                    if(row<clipboard_history_count() && row<6u) clipboard_restore(row);
                } else if(w->app==APP_LAUNCHER && pointer_y>=view.y+80u) {
                    uint32_t row=(pointer_y-(view.y+80u))/38u;
                    launch_result(row);
                } else if(w->app==APP_NOTIFICATIONS && pointer_y>=view.y+58u) {
                    uint32_t row=(pointer_y-(view.y+58u))/42u;
                    if(row<notification_count() && row<5u) notification_dismiss(row);
                } else if(w->app==APP_APPEARANCE && pointer_y>=view.y+58u) {
                    if(pointer_x>=view.x+12u) {
                        uint32_t column=(pointer_x-(view.x+12u))/152u;
                        uint32_t row=(pointer_y-(view.y+58u))/18u;
                        uint32_t index=column*12u+row;
                        const struct bread_theme *choice=theme_at(index);
                        if(choice) theme_apply(choice->name);
                    }
                }
                break;
            }
        }
    }
    if(pressed && dragging>=0 && workspace_layout[active_workspace]==LAYOUT_FLOAT) {
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

static void render_screensaver(void) {
    const struct bread_theme *theme=theme_current();
    video_clear(theme->background);
    video_draw_text(screen_width/2u-56u,screen_height/2u-24u,
                    "BREADOS",theme->accent,theme->background);
    video_draw_text(screen_width/2u-104u,screen_height/2u+4u,
                    "Press a key or move mouse",theme->foreground,theme->background);
}

int desktop_run(void) {
    if(!video_available()) return -1;
    screen_width=video_width();screen_height=video_height();
    if(screen_width<320u || screen_height<240u) return -1;
    for(uint32_t i=0;i<DESKTOP_WINDOWS;i++) windows[i].open=0;
    active_workspace=0;focused_window=DESKTOP_WINDOWS;dragging=-1;previous_buttons=0;selected_file=0;
    for(uint32_t i=0;i<DESKTOP_WORKSPACES;i++){workspace_layout[i]=LAYOUT_DWINDLE;workspace_scroll[i]=0;}
    pointer_x=screen_width/2u;pointer_y=screen_height/2u;
    idle_init(timer_ticks());
    open_app(APP_SHELL);
    open_app(APP_SYSTEM);
    render();
    int saver_drawn=0;
    for(;;) {
        uint32_t now=timer_ticks();
        int changed=reminder_poll(now)?1:0;
        struct mouse_event event;
        while(mouse_read_event(&event)){
            if(idle_is_active()) {idle_activity(now);changed=1;}
            else {idle_activity(now);mouse_event(&event);changed=1;}
        }
        char key;
        while(keyboard_read_char(&key)) {
            now=timer_ticks();
            if(idle_is_active()) {idle_activity(now);changed=1;continue;}
            idle_activity(now);
            if(focused_window<DESKTOP_WINDOWS && windows[focused_window].open && windows[focused_window].app==APP_LAUNCHER) {
                if(key=='\n' || key=='\r') launch_result(0);
                else if(key=='\b') {size_t length=0;while(launcher_query[length])length++;if(length)launcher_query[length-1u]=0;}
                else if(key>=32 && key<=126) {size_t length=0;while(launcher_query[length])length++;if(length<sizeof(launcher_query)-1u){launcher_query[length]=key;launcher_query[length+1u]=0;}}
                changed=1;continue;
            }
            if(key==' ') {open_app(APP_LAUNCHER);changed=1;}
            else if(key=='q' || key=='Q') return 0;
            if(key=='t' || key=='T'){open_app(APP_APPEARANCE);changed=1;}
            else if(key=='f' || key=='F'){open_app(APP_FILES);changed=1;}
            else if(key=='s' || key=='S'){open_app(APP_SYSTEM);changed=1;}
            else if(key=='e' || key=='E'){open_app(APP_SHELL);changed=1;}
            else if(key=='n' || key=='N'){open_app(APP_NOTIFICATIONS);changed=1;}
            else if(key=='c' || key=='C'){open_app(APP_CLIPBOARD);changed=1;}
            else if(key=='d' || key=='D'){notification_set_dnd(!notification_dnd());changed=1;}
            else if(key=='[' && active_workspace) {switch_workspace(active_workspace-1u);changed=1;}
            else if(key==']' && active_workspace+1u<DESKTOP_WORKSPACES){switch_workspace(active_workspace+1u);changed=1;}
            else if(key=='l' || key=='L'){workspace_layout[active_workspace]=(uint8_t)((workspace_layout[active_workspace]+1u)%3u);workspace_scroll[active_workspace]=0;dragging=-1;changed=1;}
            else if(key==',' && workspace_layout[active_workspace]==LAYOUT_SCROLL){workspace_scroll[active_workspace]-=56;if(workspace_scroll[active_workspace]<0)workspace_scroll[active_workspace]=0;changed=1;}
            else if(key=='.' && workspace_layout[active_workspace]==LAYOUT_SCROLL){uint32_t count=0;for(uint32_t i=0;i<DESKTOP_WINDOWS;i++)if(windows[i].open && windows[i].workspace==active_workspace)count++;int32_t maximum=count?(int32_t)((count-1u)*56u):0;if(workspace_scroll[active_workspace]<maximum){workspace_scroll[active_workspace]+=56;if(workspace_scroll[active_workspace]>maximum)workspace_scroll[active_workspace]=maximum;changed=1;}}
            else if(key>='1' && key<='4'){switch_workspace((uint32_t)(key-'1'));changed=1;}
        }
        if(idle_poll(timer_ticks())) {
            if(!saver_drawn || changed) render_screensaver();
            saver_drawn=1;
            __asm__ volatile("hlt");
            continue;
        }
        if(saver_drawn){saver_drawn=0;changed=1;}
        if(changed) render();
        __asm__ volatile("hlt");
    }
}

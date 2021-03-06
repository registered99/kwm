#ifndef TYPES_H
#define TYPES_H

#include <Carbon/Carbon.h>

#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libproc.h>
#include <signal.h>

#include <pthread.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

struct hotkey;
struct modifiers;
struct container_offset;
struct color;

struct window_info;
struct window_role;
struct screen_info;
struct space_info;
struct node_container;
struct tree_node;

struct kwm_mach;
struct kwm_border;
struct kwm_hotkeys;
struct kwm_prefix;
struct kwm_toggles;
struct kwm_path;
struct kwm_focus;
struct kwm_screen;
struct kwm_tiling;
struct kwm_cache;
struct kwm_mode;
struct kwm_thread;

#ifdef DEBUG_BUILD
    #define DEBUG(x) std::cout << x << std::endl;
    #define Assert(Expression, Function) if(!(Expression)) \
                                         {\
                                            std::cout << "Assertion failed: " << Function << std::endl;\
                                            *(volatile int*)0 = 0;\
                                         }
#else
    #define DEBUG(x)
    #define Assert(Expression, Function)
#endif

#define BSP_WINDOW_EVENT_CALLBACK(name) void name(window_info *Window, int OpenWindows)
typedef BSP_WINDOW_EVENT_CALLBACK(OnBSPWindowCreate);
typedef BSP_WINDOW_EVENT_CALLBACK(OnBSPWindowDestroy);

typedef std::chrono::time_point<std::chrono::steady_clock> kwm_time_point;

#define CGSSpaceTypeUser 0
extern "C" int CGSGetActiveSpace(int cid);
extern "C" int CGSSpaceGetType(int cid, int sid);
extern "C" bool CGSManagedDisplayIsAnimating(const int cid, CFStringRef display);
extern "C" CFStringRef CGSCopyManagedDisplayForSpace(const int cid, int space);
extern "C" CFStringRef CGSCopyBestManagedDisplayForRect(const int cid, CGRect rect);
extern "C" CFArrayRef CGSCopyManagedDisplaySpaces(const int cid);

#define CGSDefaultConnection _CGSDefaultConnection()
extern "C" int _CGSDefaultConnection(void);

extern "C" AXError _AXUIElementGetWindow(AXUIElementRef, int *);

enum focus_option
{
    FocusModeAutofocus,
    FocusModeAutoraise,
    FocusModeStandby,
    FocusModeDisabled
};

enum cycle_focus_option
{
    CycleModeScreen,
    CycleModeAll,
    CycleModeDisabled
};

enum space_tiling_option
{
    SpaceModeBSP,
    SpaceModeMonocle,
    SpaceModeFloating,
    SpaceModeDefault
};

enum split_type
{
    SPLIT_OPTIMAL = -1,
    SPLIT_VERTICAL = 1,
    SPLIT_HORIZONTAL = 2
};

enum hotkey_state
{
    HotkeyStateNone,
    HotkeyStateInclude,
    HotkeyStateExclude
};

struct modifiers
{
    bool CmdKey;
    bool AltKey;
    bool CtrlKey;
    bool ShiftKey;
};

struct hotkey
{
    std::vector<std::string> List;
    bool IsSystemCommand;
    hotkey_state State;

    modifiers Mod;
    CGKeyCode Key;
    bool Prefixed;

    std::string Command;
};

struct container_offset
{
    double PaddingTop, PaddingBottom;
    double PaddingLeft, PaddingRight;
    double VerticalGap, HorizontalGap;
};

struct color
{
    double Red;
    double Green;
    double Blue;
    double Alpha;

    std::string Format;
};

struct node_container
{
    double X, Y;
    double Width, Height;
    int Type;
};

struct tree_node
{
    int WindowID;
    node_container Container;
    tree_node *Parent;
    tree_node *LeftChild;
    tree_node *RightChild;

    split_type SplitMode;
    double SplitRatio;
};

struct window_info
{
    std::string Name;
    std::string Owner;
    int PID, WID;
    int Layer;
    int X, Y;
    int Width, Height;
};

struct window_role
{
    CFTypeRef Role;
    CFTypeRef SubRole;
};

struct space_info
{
    container_offset Offset;
    bool Initialized;
    bool Managed;

    space_tiling_option Mode;
    tree_node *FocusedNode;
    tree_node *RootNode;
};

struct screen_info
{
    CFStringRef Identifier;
    unsigned int ID;

    int X, Y;
    double Width, Height;
    container_offset Offset;

    int ActiveSpace;
    int OldWindowListCount;
    std::map<int, space_info> Space;
};

struct kwm_mach
{
    void *WorkspaceWatcher;
    CFRunLoopSourceRef RunLoopSource;
    CFMachPortRef EventTap;
    CGEventMask EventMask;
};

struct kwm_border
{
    bool Enabled;
    FILE *Handle;

    color Color;
    double Radius;
    int Width;
};

struct kwm_prefix
{
    kwm_time_point Time;
    hotkey Key;

    double Timeout;
    bool Enabled;
    bool Active;
    bool Global;
};


struct kwm_hotkeys
{
    std::vector<hotkey> List;
    kwm_prefix Prefix;
    modifiers SpacesKey;
};

struct kwm_toggles
{
    bool UseMouseFollowsFocus;
    bool EnableTilingMode;
    bool UseBuiltinHotkeys;
    bool StandbyOnFloat;
};

struct kwm_path
{
    std::string EnvHome;
    std::string FilePath;
    std::string ConfigFolder;
    std::string ConfigFile;
    std::string BSPLayouts;
};

struct kwm_focus
{
    AXObserverRef Observer;
    AXUIElementRef Application;

    ProcessSerialNumber PSN;
    window_info *Window;
    window_info Cache;
    window_info NULLWindowInfo;
};

struct kwm_screen
{
    screen_info *Current;
    bool Transitioning;
    double SplitRatio;

    int MarkedWindow;
    split_type SplitMode;
    int PrevSpace;

    container_offset DefaultOffset;
    CGDirectDisplayID *Displays;
    unsigned int MaxCount;
    unsigned int ActiveCount;
};

struct kwm_tiling
{
    bool SpawnAsLeftChild;
    bool FloatNonResizable;
    double OptimalRatio;

    std::map<unsigned int, screen_info> DisplayMap;
    std::map<unsigned int, space_tiling_option> DisplayMode;

    std::map<std::string, std::vector<CFTypeRef> > AllowedWindowRoles;
    std::map<std::string, int> CapturedAppLst;
    std::vector<std::string> FloatingAppLst;

    std::vector<window_info> FocusLst;
    std::vector<window_info> WindowLst;
    std::vector<int> FloatingWindowLst;
};

struct kwm_cache
{
    std::map<int, window_role> WindowRole;
    std::map<int, std::vector<AXUIElementRef> > WindowRefs;
};

struct kwm_mode
{
    space_tiling_option Space;
    cycle_focus_option Cycle;
    focus_option Focus;
};

struct kwm_thread
{
    pthread_t WindowMonitor;
    pthread_t SystemCommand;
    pthread_t Daemon;
    pthread_mutex_t Lock;
};

struct kwm_callback
{
    OnBSPWindowCreate *WindowCreate;
    OnBSPWindowDestroy *WindowDestroy;
};

#endif

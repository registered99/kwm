#include "space.h"
#include "display.h"
#include "window.h"
#include "tree.h"
#include "border.h"
#include "keys.h"

extern kwm_mach KWMMach;
extern kwm_screen KWMScreen;
extern kwm_focus KWMFocus;
extern kwm_toggles KWMToggles;
extern kwm_mode KWMMode;
extern kwm_thread KWMThread;
extern kwm_border FocusedBorder;
extern kwm_border MarkedBorder;

void GetTagForMonocleSpace(space_info *Space, std::string &Tag)
{
    tree_node *Node = Space->RootNode;
    bool FoundFocusedWindow = false;
    int FocusedIndex = 0;
    int NumberOfWindows = 0;

    if(Node && KWMFocus.Window)
    {
        FocusedIndex = 1;
        NumberOfWindows = 1;

        if(Node->WindowID == KWMFocus.Window->WID)
            FoundFocusedWindow = true;

        while(Node->RightChild)
        {
            if(Node->WindowID == KWMFocus.Window->WID)
                FoundFocusedWindow = true;

            if(!FoundFocusedWindow)
                ++FocusedIndex;

            ++NumberOfWindows;

            Node = Node->RightChild;
        }

        if(Node->WindowID == KWMFocus.Window->WID)
            FoundFocusedWindow = true;
    }

    if(FoundFocusedWindow)
        Tag = "[" + std::to_string(FocusedIndex) + "/" + std::to_string(NumberOfWindows) + "]";
    else
        Tag = "[" + std::to_string(NumberOfWindows) + "]";
}

void GetTagForCurrentSpace(std::string &Tag)
{
    if(IsSpaceInitializedForScreen(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        if(Space->Mode == SpaceModeBSP)
            Tag = "[bsp]";
        else if(Space->Mode == SpaceModeFloating)
            Tag = "[float]";
        else if(Space->Mode == SpaceModeMonocle)
            GetTagForMonocleSpace(Space, Tag);
    }
    else
    {
        if(KWMMode.Space == SpaceModeBSP)
            Tag = "[bsp]";
        else if(KWMMode.Space == SpaceModeFloating)
            Tag = "[float]";
        else if(KWMMode.Space == SpaceModeMonocle)
            Tag = "[monocle]";
    }
}

void MoveWindowToSpace(std::string SpaceID)
{
    if(!KWMFocus.Window)
        return;

    CGEventTapEnable(KWMMach.EventTap, false);

    window_info *Window = KWMFocus.Window;
    bool WasWindowFloating = IsFocusedWindowFloating();
    if(!WasWindowFloating)
    {
        screen_info *ScreenOfWindow = GetDisplayOfWindow(Window);
        space_info *SpaceOfWindow = GetActiveSpaceOfScreen(ScreenOfWindow);
        if(SpaceOfWindow->Mode == SpaceModeBSP)
            RemoveWindowFromBSPTree(ScreenOfWindow, Window->WID, false, false);
        else if(SpaceOfWindow->Mode == SpaceModeMonocle)
            RemoveWindowFromMonocleTree(ScreenOfWindow, Window->WID, false);
    }

    CGPoint CursorPos = GetCursorPos();
    CGPoint WindowPos = GetWindowPos(Window);
    CGPoint ClickPos = CGPointMake(WindowPos.x + 75, WindowPos.y + 7);

    CGEventRef ClickEvent = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, ClickPos, kCGMouseButtonLeft);
    CGEventSetFlags(ClickEvent, 0);
    CGEventPost(kCGHIDEventTap, ClickEvent);
    CFRelease(ClickEvent);
    usleep(150000);

    KwmEmitKeystroke(KWMHotkeys.SpacesKey, SpaceID);
    usleep(300000);

    KWMScreen.PrevSpace = KWMScreen.Current->ActiveSpace;
    KWMScreen.Current->ActiveSpace = GetActiveSpaceOfDisplay(KWMScreen.Current);
    ShouldActiveSpaceBeManaged();
    UpdateActiveWindowList(KWMScreen.Current);

    ClickEvent = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, ClickPos, kCGMouseButtonLeft);
    CGEventSetFlags(ClickEvent, 0);
    CGEventPost(kCGHIDEventTap, ClickEvent);
    CFRelease(ClickEvent);
    usleep(150000);

    if(!WasWindowFloating)
    {
        screen_info *ScreenOfWindow = GetDisplayOfWindow(Window);
        space_info *SpaceOfWindow = GetActiveSpaceOfScreen(ScreenOfWindow);
        if(SpaceOfWindow->Mode == SpaceModeBSP)
            AddWindowToBSPTree(ScreenOfWindow, Window->WID);
        else if(SpaceOfWindow->Mode == SpaceModeMonocle)
            AddWindowToMonocleTree(ScreenOfWindow, Window->WID);
    }

    CGWarpMouseCursorPosition(CursorPos);
    CGEventTapEnable(KWMMach.EventTap, true);
}

bool IsActiveSpaceFloating()
{
    return IsSpaceFloating(KWMScreen.Current->ActiveSpace);
}

bool IsSpaceFloating(int SpaceID)
{
    bool Result = false;

    if(IsSpaceInitializedForScreen(KWMScreen.Current))
    {
        std::map<int, space_info>::iterator It = KWMScreen.Current->Space.find(SpaceID);
        if(It != KWMScreen.Current->Space.end())
            Result = KWMScreen.Current->Space[SpaceID].Mode == SpaceModeFloating;
    }

    return Result;
}

space_info *GetActiveSpaceOfScreen(screen_info *Screen)
{
    space_info *Space = NULL;
    std::map<int, space_info>::iterator It = Screen->Space.find(Screen->ActiveSpace);

    if(It == Screen->Space.end())
    {
        space_info Clear = {{0}};
        Screen->ActiveSpace = GetActiveSpaceOfDisplay(Screen);
        Screen->Space[Screen->ActiveSpace] = Clear;
        Space = &Screen->Space[Screen->ActiveSpace];
    }
    else
    {
        Space = &Screen->Space[Screen->ActiveSpace];
    }

    return Space;
}

bool IsSpaceInitializedForScreen(screen_info *Screen)
{
    if(!Screen)
        return false;

    std::map<int, space_info>::iterator It = Screen->Space.find(Screen->ActiveSpace);
    if(It == Screen->Space.end())
        return false;
    else
        return It->second.Initialized;
}

bool DoesSpaceExistInMapOfScreen(screen_info *Screen)
{
    if(!Screen)
        return false;

    std::map<int, space_info>::iterator It = Screen->Space.find(Screen->ActiveSpace);
    if(It == Screen->Space.end())
        return false;
    else
        return It->second.RootNode != NULL && It->second.Initialized;
}

bool IsSpaceTransitionInProgress()
{
    if(KWMScreen.Transitioning)
        return true;

    Assert(KWMScreen.Current, "IsSpaceTransitionInProgress()")
    if(!KWMScreen.Current->Identifier)
        KWMScreen.Current->Identifier = GetDisplayIdentifier(KWMScreen.Current);

    bool Result = CGSManagedDisplayIsAnimating(CGSDefaultConnection, KWMScreen.Current->Identifier);
    if(Result)
    {
        DEBUG("IsSpaceTransitionInProgress() Space transition detected")
        KWMScreen.Transitioning = true;
        ClearFocusedWindow();
        ClearMarkedWindow();
    }

    return Result;
}

bool IsActiveSpaceManaged()
{
    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    return Space->Managed;
}

void ShouldActiveSpaceBeManaged()
{
    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    Space->Managed = CGSSpaceGetType(CGSDefaultConnection, KWMScreen.Current->ActiveSpace) == CGSSpaceTypeUser;
}

void FloatFocusedSpace()
{
    if(KWMToggles.EnableTilingMode &&
       !IsSpaceTransitionInProgress() &&
       IsActiveSpaceManaged())
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        if(Space->Mode == SpaceModeFloating)
            return;

        DestroyNodeTree(Space->RootNode, Space->Mode);
        Space->RootNode = NULL;

        Space->Mode = SpaceModeFloating;
        Space->Initialized = true;
        ClearFocusedWindow();
    }
}

void TileFocusedSpace(space_tiling_option Mode)
{
    if(KWMToggles.EnableTilingMode &&
       !IsSpaceTransitionInProgress() &&
       IsActiveSpaceManaged() &&
       FilterWindowList(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        if(Space->Mode == Mode)
            return;

        DestroyNodeTree(Space->RootNode, Space->Mode);
        Space->RootNode = NULL;

        Space->Mode = Mode;
        std::vector<window_info*> WindowsOnDisplay = GetAllWindowsOnDisplay(KWMScreen.Current->ID);
        CreateWindowNodeTree(KWMScreen.Current, &WindowsOnDisplay);
    }
}

void ToggleFocusedSpaceFloating()
{
    if(!IsSpaceFloating(KWMScreen.Current->ActiveSpace))
        FloatFocusedSpace();
    else
        TileFocusedSpace(SpaceModeBSP);
}

void UpdateActiveSpace()
{
    pthread_mutex_lock(&KWMThread.Lock);
    Assert(KWMScreen.Current, "UpdateActiveSpace()")

    KWMScreen.Transitioning = false;
    KWMScreen.PrevSpace = KWMScreen.Current->ActiveSpace;
    KWMScreen.Current->ActiveSpace = GetActiveSpaceOfDisplay(KWMScreen.Current);
    ShouldActiveSpaceBeManaged();

    if(KWMScreen.PrevSpace != KWMScreen.Current->ActiveSpace)
    {
        DEBUG("UpdateActiveSpace() Space transition ended " << KWMScreen.PrevSpace << " -> " << KWMScreen.Current->ActiveSpace)

        UpdateActiveWindowList(KWMScreen.Current);
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        if(Space->FocusedNode)
        {
            SetWindowFocusByNode(Space->FocusedNode);
            MoveCursorToCenterOfFocusedWindow();
        }
    }

    pthread_mutex_unlock(&KWMThread.Lock);
}

#include <fbxvpch.h>
#include <Input.h>

void BuildSdlKeyMapping(uint32_t * pOutKeyMapping)
{
    using namespace fbxv;
    //TIMED_SCOPE(timer, "InputKeyMapping");
     
    pOutKeyMapping[kDigitalInput_KeyEscape]       = SDL_SCANCODE_ESCAPE;
    pOutKeyMapping[kDigitalInput_Key1]            = SDL_SCANCODE_1;
    pOutKeyMapping[kDigitalInput_Key2]            = SDL_SCANCODE_2;
    pOutKeyMapping[kDigitalInput_Key3]            = SDL_SCANCODE_3;
    pOutKeyMapping[kDigitalInput_Key4]            = SDL_SCANCODE_4;
    pOutKeyMapping[kDigitalInput_Key5]            = SDL_SCANCODE_5;
    pOutKeyMapping[kDigitalInput_Key6]            = SDL_SCANCODE_6;
    pOutKeyMapping[kDigitalInput_Key7]            = SDL_SCANCODE_7;
    pOutKeyMapping[kDigitalInput_Key8]            = SDL_SCANCODE_8;
    pOutKeyMapping[kDigitalInput_Key9]            = SDL_SCANCODE_9;
    pOutKeyMapping[kDigitalInput_Key0]            = SDL_SCANCODE_0;
    pOutKeyMapping[kDigitalInput_KeyMinus]        = SDL_SCANCODE_MINUS;
    pOutKeyMapping[kDigitalInput_KeyEquals]       = SDL_SCANCODE_EQUALS;
    pOutKeyMapping[kDigitalInput_KeyBack]         = SDL_SCANCODE_AC_BACK;
    pOutKeyMapping[kDigitalInput_KeyTab]          = SDL_SCANCODE_TAB;
    pOutKeyMapping[kDigitalInput_KeyQ]            = SDL_SCANCODE_Q;
    pOutKeyMapping[kDigitalInput_KeyW]            = SDL_SCANCODE_W;
    pOutKeyMapping[kDigitalInput_KeyE]            = SDL_SCANCODE_E;
    pOutKeyMapping[kDigitalInput_KeyR]            = SDL_SCANCODE_R;
    pOutKeyMapping[kDigitalInput_KeyT]            = SDL_SCANCODE_T;
    pOutKeyMapping[kDigitalInput_KeyY]            = SDL_SCANCODE_Y;
    pOutKeyMapping[kDigitalInput_KeyU]            = SDL_SCANCODE_U;
    pOutKeyMapping[kDigitalInput_KeyI]            = SDL_SCANCODE_I;
    pOutKeyMapping[kDigitalInput_KeyO]            = SDL_SCANCODE_O;
    pOutKeyMapping[kDigitalInput_KeyP]            = SDL_SCANCODE_P;
    pOutKeyMapping[kDigitalInput_KeyLeftBracket]  = SDL_SCANCODE_LEFTBRACKET;
    pOutKeyMapping[kDigitalInput_KeyRightBracket] = SDL_SCANCODE_RIGHTBRACKET;
    pOutKeyMapping[kDigitalInput_KeyReturn]       = SDL_SCANCODE_RETURN;
    pOutKeyMapping[kDigitalInput_KeyLeftControl]  = SDL_SCANCODE_LCTRL;
    pOutKeyMapping[kDigitalInput_KeyA]            = SDL_SCANCODE_A;
    pOutKeyMapping[kDigitalInput_KeyS]            = SDL_SCANCODE_S;
    pOutKeyMapping[kDigitalInput_KeyD]            = SDL_SCANCODE_D;
    pOutKeyMapping[kDigitalInput_KeyF]            = SDL_SCANCODE_F;
    pOutKeyMapping[kDigitalInput_KeyG]            = SDL_SCANCODE_G;
    pOutKeyMapping[kDigitalInput_KeyH]            = SDL_SCANCODE_H;
    pOutKeyMapping[kDigitalInput_KeyJ]            = SDL_SCANCODE_J;
    pOutKeyMapping[kDigitalInput_KeyK]            = SDL_SCANCODE_K;
    pOutKeyMapping[kDigitalInput_KeyL]            = SDL_SCANCODE_L;
    pOutKeyMapping[kDigitalInput_KeySemicolon]    = SDL_SCANCODE_SEMICOLON;
    pOutKeyMapping[kDigitalInput_KeyApostrophe]   = SDL_SCANCODE_APOSTROPHE;
    pOutKeyMapping[kDigitalInput_KeyGrave]        = SDL_SCANCODE_GRAVE;
    pOutKeyMapping[kDigitalInput_KeyLeftShift]    = SDL_SCANCODE_LSHIFT;
    pOutKeyMapping[kDigitalInput_KeyBackslash]    = SDL_SCANCODE_BACKSLASH;
    pOutKeyMapping[kDigitalInput_KeyZ]            = SDL_SCANCODE_Z;
    pOutKeyMapping[kDigitalInput_KeyX]            = SDL_SCANCODE_X;
    pOutKeyMapping[kDigitalInput_KeyC]            = SDL_SCANCODE_C;
    pOutKeyMapping[kDigitalInput_KeyV]            = SDL_SCANCODE_V;
    pOutKeyMapping[kDigitalInput_KeyB]            = SDL_SCANCODE_B;
    pOutKeyMapping[kDigitalInput_KeyN]            = SDL_SCANCODE_N;
    pOutKeyMapping[kDigitalInput_KeyM]            = SDL_SCANCODE_M;
    pOutKeyMapping[kDigitalInput_KeyComma]        = SDL_SCANCODE_COMMA;
    pOutKeyMapping[kDigitalInput_KeyPeriod]       = SDL_SCANCODE_PERIOD;
    pOutKeyMapping[kDigitalInput_KeySlash]        = SDL_SCANCODE_SLASH;
    pOutKeyMapping[kDigitalInput_KeyRightShift]   = SDL_SCANCODE_RSHIFT;
    pOutKeyMapping[kDigitalInput_KeyLeftAlt]      = SDL_SCANCODE_LALT;
    pOutKeyMapping[kDigitalInput_KeySpace]        = SDL_SCANCODE_SPACE;
    pOutKeyMapping[kDigitalInput_KeyCapital]      = SDL_SCANCODE_CAPSLOCK;
    pOutKeyMapping[kDigitalInput_KeyF1]           = SDL_SCANCODE_F1;
    pOutKeyMapping[kDigitalInput_KeyF2]           = SDL_SCANCODE_F2;
    pOutKeyMapping[kDigitalInput_KeyF3]           = SDL_SCANCODE_F3;
    pOutKeyMapping[kDigitalInput_KeyF4]           = SDL_SCANCODE_F4;
    pOutKeyMapping[kDigitalInput_KeyF5]           = SDL_SCANCODE_F5;
    pOutKeyMapping[kDigitalInput_KeyF6]           = SDL_SCANCODE_F6;
    pOutKeyMapping[kDigitalInput_KeyF7]           = SDL_SCANCODE_F7;
    pOutKeyMapping[kDigitalInput_KeyF8]           = SDL_SCANCODE_F8;
    pOutKeyMapping[kDigitalInput_KeyF9]           = SDL_SCANCODE_F9;
    pOutKeyMapping[kDigitalInput_KeyF]            = SDL_SCANCODE_F;
    pOutKeyMapping[kDigitalInput_KeyF10]          = SDL_SCANCODE_F10;
    pOutKeyMapping[kDigitalInput_KeyF11]          = SDL_SCANCODE_F11;
    pOutKeyMapping[kDigitalInput_KeyF12]          = SDL_SCANCODE_F12;
    pOutKeyMapping[kDigitalInput_KeyRightControl] = SDL_SCANCODE_RCTRL;
    pOutKeyMapping[kDigitalInput_KeyRightAlt]     = SDL_SCANCODE_RALT;
    pOutKeyMapping[kDigitalInput_KeyHome]         = SDL_SCANCODE_HOME;
    pOutKeyMapping[kDigitalInput_KeyUp]           = SDL_SCANCODE_UP;
    pOutKeyMapping[kDigitalInput_KeyLeft]         = SDL_SCANCODE_LEFT;
    pOutKeyMapping[kDigitalInput_KeyRight]        = SDL_SCANCODE_RIGHT;
    pOutKeyMapping[kDigitalInput_KeyEnd]          = SDL_SCANCODE_END;
    pOutKeyMapping[kDigitalInput_KeyDown]         = SDL_SCANCODE_DOWN;
    pOutKeyMapping[kDigitalInput_KeyInsert]       = SDL_SCANCODE_INSERT;
    pOutKeyMapping[kDigitalInput_KeyDelete]       = SDL_SCANCODE_DELETE;
}

fbxv::Input::Input ()
    : bFocused (false)
    , bSizeChanged (false)
    , bIsUsingTouch (false)
    , bIsAnyPressed (false)
    , bIsQuitRequested (false)
    , bIsTrackingTouchesOrMousePressed (false)
{
    memset (Buttons, 0, sizeof (Buttons));
    memset (HoldDuration, 0, sizeof (HoldDuration));
    memset (Analogs, 0, sizeof (Analogs));
    memset (AnalogsTimeCorrected, 0, sizeof (AnalogsTimeCorrected));

    TouchIdCount = 0;
    for (auto & TouchId : TouchIds)
        TouchId = sInvalidTouchValue;
}

fbxv::Input::~Input()
{
}

bool fbxv::Input::IsTouchEnabled() const
{
    return bIsUsingTouch;
}

uint32_t fbxv::Input::GetFirstTouchId() const
{
    assert(TouchIdCount > 0 && "Invalid.");
    return TouchIds[0];
}

uint32_t fbxv::Input::GetLastTouchId() const
{
    assert(TouchIdCount > 0 && "Out of range.");
    return TouchIds[TouchIdCount - 1];
}

bool fbxv::Input::IsTouchTracked(uint32_t TouchId) const
{
    const auto TouchIt    = TouchIds;
    const auto TouchItEnd = TouchIds + TouchIdCount;
    return std::find(TouchIt, TouchItEnd, TouchId) != TouchItEnd;
}

uint32_t fbxv::Input::GetTouchCount() const
{
    return TouchIdCount;
}

bool fbxv::Input::IsAnyPressed() const
{
    return bIsAnyPressed;
}

bool fbxv::Input::IsTrackingTouchesOrMousePressed() const
{
    return bIsTrackingTouchesOrMousePressed;
}

bool fbxv::Input::IsPressed (DigitalInputUInt InDigitalInput) const
{
    return Buttons[ 0 ][ InDigitalInput ];
}

bool fbxv::Input::IsFirstPressed (DigitalInputUInt InDigitalInput) const
{
    return Buttons[ 0 ][ InDigitalInput ] && !Buttons[ 1 ][ InDigitalInput ];
}

bool fbxv::Input::IsReleased (DigitalInputUInt InDigitalInput) const
{
    return !Buttons[ 0 ][ InDigitalInput ];
}

bool fbxv::Input::IsFirstReleased (DigitalInputUInt InDigitalInput) const
{
    return !Buttons[ 0 ][ InDigitalInput ] && Buttons[ 1 ][ InDigitalInput ];
}

float fbxv::Input::GetDurationPressed(DigitalInputUInt InDigitalInput) const
{
    return HoldDuration[InDigitalInput];
}

float fbxv::Input::GetAnalogInput (AnalogInputUInt InAnalogInput) const
{
    return Analogs[ InAnalogInput ];
}

float fbxv::Input::GetTimeCorrectedAnalogInput (AnalogInputUInt InAnalogInput) const
{
    return AnalogsTimeCorrected[ InAnalogInput ];
}

fbxv::InputManager::InputManager ()
{
}

fbxv::InputManager::~InputManager ()
{
}

bool fbxv::InputManager::Initialize ()
{
    BuildSdlKeyMapping (KeyMapping);
    return true;
}

static void LogTouches (uint32_t stateIndex, bool const * touchButtonsState)
{
    SDL_LogInfo (SDL_LOG_CATEGORY_APPLICATION,
                 "Octopus: \t[%u]"
                 " %u %u %u %u"
                 " %u %u %u %u"
                 " %u %u %u %u"
                 " %u %u %u %u",
                 stateIndex,
                 touchButtonsState[ 0 ],
                 touchButtonsState[ 1 ],
                 touchButtonsState[ 2 ],
                 touchButtonsState[ 3 ],
                 touchButtonsState[ 4 ],
                 touchButtonsState[ 5 ],
                 touchButtonsState[ 6 ],
                 touchButtonsState[ 7 ],
                 touchButtonsState[ 8 ],
                 touchButtonsState[ 9 ],
                 touchButtonsState[ 10 ],
                 touchButtonsState[ 11 ],
                 touchButtonsState[ 12 ],
                 touchButtonsState[ 13 ],
                 touchButtonsState[ 14 ],
                 touchButtonsState[ 15 ]);
}

void fbxv::InputManager::Update(Input & InOutState, float const DeltaTime)
{
    SDL_PumpEvents();

    static SDL_Event windowEvents[ 16 ];

    InOutState.bSizeChanged = false;

    if (const int windowEventCount
        = std::min<int> (SDL_PeepEvents (windowEvents,
                                         _Get_array_length (windowEvents),
                                         SDL_PEEKEVENT,
                                         SDL_WINDOWEVENT,
                                         SDL_WINDOWEVENT),
                         _Get_array_length (windowEvents)))
    {
        for (int i = 0; i < windowEventCount; ++i)
        {
            SDL_Event const & windowEvent = windowEvents[ i ];
            if (windowEvent.window.type == SDL_WINDOWEVENT_SIZE_CHANGED
                || windowEvent.window.type == SDL_WINDOWEVENT_RESIZED)
            {
                SDL_LogWarn (SDL_LOG_CATEGORY_APPLICATION, "fbxv: Window size changed.");
                InOutState.bSizeChanged = true;
            }
            else if (windowEvent.window.type == SDL_WINDOWEVENT_FOCUS_GAINED)
            {
                SDL_LogWarn (SDL_LOG_CATEGORY_APPLICATION, "fbxv: Window gained focus.");
                InOutState.bFocused = true;
            }
            else if (windowEvent.window.type == SDL_WINDOWEVENT_FOCUS_LOST)
            {
                SDL_LogWarn (SDL_LOG_CATEGORY_APPLICATION, "fbxv: Window lost focus.");
                InOutState.bFocused = false;
            }
        }
    }

    InOutState.bIsQuitRequested = SDL_PeepEvents(nullptr, 0, SDL_PEEKEVENT, SDL_QUIT, SDL_QUIT) > 0;

    // Preserve previous values
    memcpy(InOutState.Buttons[1], InOutState.Buttons[0], sizeof(InOutState.Buttons[0]));

    // Clear tracking flags.
    InOutState.bIsAnyPressed                    = false;
    InOutState.bIsTrackingTouchesOrMousePressed = false;

    // Clear dynamic per-frame values.
    memset(InOutState.Buttons[0], 0, sizeof(bool) * (kDigitalInput_Touch0));
    memset(InOutState.Analogs, 0, sizeof(bool) * kAnalogInput_Touch0X);

    if (auto SdlKeybuffer = SDL_GetKeyboardState(nullptr))
    {
        for (uint32_t BtnIdx = 0; BtnIdx < kDigitalInput_NumKeys; ++BtnIdx)
        {
            const bool IsBtnPressed = SdlKeybuffer[KeyMapping[BtnIdx]] != 0;

            InOutState.Buttons[0][BtnIdx] = IsBtnPressed;
            InOutState.bIsAnyPressed |= IsBtnPressed;
        }
    }

    if (InOutState.bIsUsingTouch = (SDL_GetNumTouchDevices() > 0))
    {
        // Since input update will be called only from main thread,
        // I made this array static, why allocate it each time.
        static SDL_Event peepedEvents[ kDigitalInput_TouchMaxCount ];

        // Peep only finger events and remove them from event queue.
        int peepedEventCount = SDL_PeepEvents (peepedEvents,
                                               kDigitalInput_TouchMaxCount,
                                               SDL_GETEVENT,
                                               SDL_FINGERDOWN,
                                               SDL_FINGERMOTION);

        if (peepedEventCount > 0)
        {
            peepedEventCount = std::min<int> (peepedEventCount, kDigitalInput_TouchMaxCount);

            for (int i = 0; i < peepedEventCount; ++i)
            {
                SDL_Event & PeepedEvent = peepedEvents[ i ];

                if (PeepedEvent.tfinger.fingerId < 0 || PeepedEvent.tfinger.fingerId > kDigitalInput_TouchMaxCount)
                {
                    SDL_LogError (SDL_LOG_CATEGORY_APPLICATION, "Octopus: Finger id is invalid.");
                    continue;
                }

                bool const bIsFingerTracked = PeepedEvent.type != SDL_FINGERUP;
                uint32_t const FinderId = static_cast<uint32_t> (PeepedEvent.tfinger.fingerId);

                InOutState.bIsAnyPressed |= bIsFingerTracked;
                InOutState.bIsTrackingTouchesOrMousePressed |= bIsFingerTracked;

                uint32_t const eFinger = kDigitalInput_Touch0 + FinderId;
                uint32_t const eTouchX = kAnalogInput_Touch0X + FinderId;
                uint32_t const eTouchY = kAnalogInput_Touch0Y + FinderId;

                InOutState.Buttons[ 0 ][ eFinger ] = bIsFingerTracked;
                InOutState.Analogs[ eTouchX ]      = PeepedEvent.tfinger.x;
                InOutState.Analogs[ eTouchY ]      = PeepedEvent.tfinger.y;

                switch (PeepedEvent.type)
                {
                case SDL_FINGERDOWN:
                {
                    InOutState.TouchIds[ InOutState.TouchIdCount ] = FinderId;

                    auto TouchItEnd = InOutState.TouchIds + InOutState.TouchIdCount + 1;
                    std::sort (InOutState.TouchIds, TouchItEnd);

                    auto TouchIt            = std::unique (InOutState.TouchIds, TouchItEnd);
                    InOutState.TouchIdCount = std::distance (InOutState.TouchIds, TouchIt);
                }
                break;

                case SDL_FINGERUP:
                {
                    auto TouchItEnd = InOutState.TouchIds + InOutState.TouchIdCount;
                    auto TouchIt    = std::find (InOutState.TouchIds, TouchItEnd, FinderId);
                    if (TouchIt != TouchItEnd)
                    {
                        uint32_t TouchIdx = std::distance (InOutState.TouchIds, TouchIt);
                        InOutState.TouchIds[ TouchIdx ] = Input::sInvalidTouchValue;

                        std::sort (InOutState.TouchIds, TouchItEnd);
                        TouchIt                 = std::unique (InOutState.TouchIds, TouchItEnd);
                        InOutState.TouchIdCount = std::distance (InOutState.TouchIds, TouchIt)
                            - ptrdiff_t (TouchItEnd == TouchIt);
                    }
                }
                break;

                case SDL_FINGERMOTION:
                    break;
                default:
                    SDL_LogError (SDL_LOG_CATEGORY_APPLICATION, "Octopus: Unknown event");
                    break;
                }
            }
        }
    }
    else
    {
        int RelativeMouseDeltaX, RelativeMouseDeltaY;
        auto const PressedMouseButtonBitmask = SDL_GetMouseState(&RelativeMouseDeltaX, &RelativeMouseDeltaY);
        bool const bIsAnyMousePressed = !!PressedMouseButtonBitmask;

        // At this point we can trigger tracking flags.
        InOutState.bIsAnyPressed |= bIsAnyMousePressed;
        InOutState.bIsTrackingTouchesOrMousePressed |= bIsAnyMousePressed;

        InOutState.Analogs[kAnalogInput_MouseX] = static_cast<float>(RelativeMouseDeltaX);
        InOutState.Analogs[kAnalogInput_MouseY] = static_cast<float>(RelativeMouseDeltaY);

        // Check each available mouse key if it is pressed.
        if ((PressedMouseButtonBitmask & SDL_BUTTON_LMASK) == SDL_BUTTON_LMASK)
            InOutState.Buttons[0][kDigitalInput_Mouse0] = true;
        if ((PressedMouseButtonBitmask & SDL_BUTTON_RMASK) == SDL_BUTTON_RMASK)
            InOutState.Buttons[0][kDigitalInput_Mouse1] = true;
        if ((PressedMouseButtonBitmask & SDL_BUTTON_MMASK) == SDL_BUTTON_MMASK)
            InOutState.Buttons[0][kDigitalInput_Mouse2] = true;
        if ((PressedMouseButtonBitmask & SDL_BUTTON_X1MASK) == SDL_BUTTON_X1MASK)
            InOutState.Buttons[0][kDigitalInput_Mouse3] = true;
        if ((PressedMouseButtonBitmask & SDL_BUTTON_X2MASK) == SDL_BUTTON_X2MASK)
            InOutState.Buttons[0][kDigitalInput_Mouse4] = true;
    }

    // Update time duration for buttons pressed
    for (uint32_t BtnIdx = 0; BtnIdx < kDigitalInput_NumInputs; ++BtnIdx)
    {
        if (!InOutState.Buttons[1][BtnIdx])
        {
            InOutState.HoldDuration[BtnIdx] = 0.0f;
        }
        else if (InOutState.Buttons[0][BtnIdx])
        {
            InOutState.HoldDuration[BtnIdx] += DeltaTime;
        }
    }

    for (uint32_t AngIdx = 0; AngIdx < kAnalogInput_NumInputs; ++AngIdx)
    {
        InOutState.AnalogsTimeCorrected[AngIdx] = InOutState.Analogs[AngIdx] * DeltaTime;
    }

    SDL_FlushEvents (SDL_FIRSTEVENT, SDL_LASTEVENT);
}

void fbxv::InputManager::Release()
{
}

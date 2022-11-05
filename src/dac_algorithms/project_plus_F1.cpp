#include "dac_algorithms/project_plus_F1.hpp"
#include "communication_protocols/joybus.hpp"

namespace DACAlgorithms {
namespace ProjectPlusF1 {

#define coord(x) ((uint8_t)(128. + 100.*x + 0.5))
#define oppositeCoord(x) -((uint8_t)x)

bool trueZPress = true;
bool ledgedashFacilitationSOCD = false;
bool LSisDTaunt = true;

// 2 IP declarations
bool left_wasPressed = false;
bool right_wasPressed = false;
bool up_wasPressed = false;
bool down_wasPressed = false;

bool left_outlawUntilRelease = false;
bool right_outlawUntilRelease = false;
bool up_outlawUntilRelease = false;
bool down_outlawUntilRelease = false;

bool cancel = false;

struct Coords {
    uint8_t x;
    uint8_t y;
};

Coords coords(float xFloat, float yFloat) {
    Coords r;
    r.x = coord(xFloat);
    r.y = coord(yFloat);
    return r;
}

GCReport getGCReport(GpioToButtonSets::F1::ButtonSet buttonSet) {
    
    buttonSet.up = buttonSet.up || buttonSet.up2;

    GpioToButtonSets::F1::ButtonSet bs = buttonSet; // Alterable copy
    
    GCReport gcReport = defaultGcReport;

    /* 2IP No reactivation */
    
    
    if (bs.left && bs.right) cancel=true;   
    if (!bs.left || !bs.right) cancel=false;
    if (up_wasPressed && bs.up && bs.down && !down_wasPressed) up_outlawUntilRelease=true;
    if (down_wasPressed && bs.up && bs.down && !up_wasPressed) down_outlawUntilRelease=true;
    
    if (!bs.left) left_outlawUntilRelease=false;
    if (!bs.right) right_outlawUntilRelease=false;
    if (!bs.up) up_outlawUntilRelease=false;
    if (!bs.down) down_outlawUntilRelease=false;

    left_wasPressed = bs.left;
    right_wasPressed = bs.right;
    up_wasPressed = bs.up;
    down_wasPressed = bs.down;

    if (cancel) bs.left=false, bs.right=false;
    if (up_outlawUntilRelease) bs.up=false;
    if (down_outlawUntilRelease) bs.down=false;

    /* Stick */

    bool vertical = bs.up || bs.down;
    bool readUp = bs.up;

    bool horizontal = bs.left || bs.right;
    bool readRight = bs.right;

    Coords xy;

    if (vertical && horizontal) {
        if (bs.l || bs.r) {
            if (bs.mx == bs.my) xy = coords(0.7, 0.7);
            else if (bs.mx) xy = coords(0.72, 0.31);
            else xy = coords(0.44, 0.72);
        }
        /* Firefox angle logic */
        else if (bs.mx != bs.my) {
            if (bs.mx) {
                if (bs.cDown) xy = coords(0.82, 0.36);
                else if (bs.cLeft) xy = coords(0.84, 0.5);
                else if (bs.cUp) xy = coords(0.77, 0.55);
                else if (bs.cRight) xy = coords(0.72, 0.61);
                else xy = coords(0.7, 0.34);
            }
            else {
                if (bs.cDown) xy = coords(0.34, 0.82);
                else if (bs.cLeft) xy = coords(0.4, 0.84);
                else if (bs.cUp) xy = coords(0.55, 0.77);
                else if (bs.cRight) xy = coords(0.62, 0.72);
                else xy = coords(0.28, 0.58);
            }
        }
        else xy = coords(0.7, 0.7);
    }
    else if (horizontal) {
        if (bs.mx == bs.my) xy = coords(1.0, 0.0);
        else if (bs.mx) xy = coords(0.7, 0.0);
        else xy = coords(0.35, 0.0);
    }
    else if (vertical) {
        if (bs.mx == bs.my) xy = coords(0.0, 1.0);
        else if (bs.mx) xy = coords(0.0, 0.6);
        else xy = coords(0.0, 0.7);
    }
    else {
        xy = coords(0.0, 0.0);
    }

    /* Ledgedash facilitation SOCD logic */
    if (ledgedashFacilitationSOCD && (buttonSet.left && buttonSet.right)) xy.x = 1.0;

    if (horizontal && !readRight) xy.x = oppositeCoord(xy.x);
    if (vertical && !readUp) xy.y = oppositeCoord(xy.y);

    gcReport.xStick = xy.x;
    gcReport.yStick = xy.y;

    /* C-Stick */

    bool cVertical = bs.cUp != bs.cDown;
    bool cHorizontal = bs.cLeft != bs.cRight;

    Coords cxy;

    if (bs.mx && bs.my) cxy = coords(0.0, 0.0);
    else if (cVertical && cHorizontal) cxy = coords(0.525, 0.85);
    else if (cHorizontal) cxy = bs.mx ? coords(0.8375, readUp ? 0.3125 : -0.3125) : coords(1.0, 0.0);
    else if (cVertical) cxy = coords(0.0, 1.0);
    else cxy = coords(0.0, 0.0);

    if (cHorizontal && bs.cLeft) cxy.x = oppositeCoord(cxy.x);
    if (cVertical && bs.cDown) cxy.y = oppositeCoord(cxy.y);

    gcReport.cxStick = cxy.x;
    gcReport.cyStick = cxy.y;

    /* Dpad */
    if (bs.mx && bs.my) {
        gcReport.dDown = bs.cDown;
        gcReport.dLeft = bs.cLeft;
        gcReport.dUp = bs.cUp;
        gcReport.dRight = bs.cRight;
    }
    gcReport.dUp = gcReport.dUp || bs.ms;
    gcReport.dDown = gcReport.dDown || (bs.ls && LSisDTaunt);

    /* Triggers */
    gcReport.analogL = bs.l ? 140 : 0;
    gcReport.analogR = bs.r ? 140 : 0;

    /* Handle changing LS to DTaunt */
    if (bs.ls && !LSisDTaunt) {
        gcReport.analogL = bs.ls ? 49 : 0;
    }

    /* Buttons */
    gcReport.a = bs.a;
    gcReport.b = bs.b;
    gcReport.x = bs.x;
    gcReport.y = bs.y;

    /* Handle true Z press */
    if (bs.z && !trueZPress) {
        gcReport.analogL = 49;
        gcReport.a = true;
    }
    else gcReport.z = bs.z;
    gcReport.l = bs.l;
    gcReport.r = bs.r;
    gcReport.start = bs.start;

    return gcReport;
}
}
}

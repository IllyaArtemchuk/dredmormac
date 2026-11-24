#include "dredmor/sprite.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace dredmor {
namespace {
// Preserve float intermediates and the original mixed float/double third
// sector. This translation unit is built with floating contraction disabled.
float hue_channel(float p,float q,float t) {
    if (t<0.f) t+=1.f;
    if (t>1.f) t-=1.f;
    if (t<1.f/6.f) return p+((q-p)*6.f)*t;
    if (t<0.5f) return q;
    if (t<2.f/3.f) return static_cast<float>(double(p)+double((q-p)*6.f)*(2.0/3.0-double(t)));
    return p;
}
std::uint8_t colour_byte(float value) {
    const float scaled=value*255.f;
    // x86 CVTTSS2SI returns integer-indefinite for NaN/out-of-range. In
    // particular, black's 0/0 saturation produces NaNs in the original.
    const auto integer=(!std::isfinite(scaled) || double(scaled)<-2147483648.0 || double(scaled)>=2147483648.0)
        ?std::numeric_limits<std::int32_t>::min():static_cast<std::int32_t>(scaled);
    return static_cast<std::uint8_t>(integer);
}
}
void tint_sprite_palette(SpritePalette& palette,std::int32_t angle) {
    if (angle==-1) return;
    if (angle < -360000 || angle > 360000) throw std::invalid_argument("sprite tint angle exceeds port work budget");
    for (std::size_t i=0;i<palette.size();i+=3) {
        const float r=float(palette[i])/255.f,g=float(palette[i+1])/255.f,b=float(palette[i+2])/255.f;
        const float maximum=std::max(std::max(r,g),b),minimum=std::min(std::min(r,g),b);
        float h=0.f;
        if (maximum!=minimum) {
            if (maximum==r) h=((g-b)*60.f)/(maximum-minimum);
            else if (maximum==g) h=((b-r)*60.f)/(maximum-minimum)+120.f;
            else h=((r-g)*60.f)/(maximum-minimum)+240.f;
            if (h<0.f) { do {h+=360.f;} while (h<0.f); }
            else if (h>=360.f) { do {h-=360.f;} while (h>=360.f); }
        }
        const float sum=maximum+minimum,l=sum*0.5f;
        const float s=l<0.5f ? (maximum-minimum)/sum
            :l==1.f ? 0.f : (maximum-minimum)/(l*-2.f+2.f);
        h+=float(angle);
        if (h<0.f) { do {h+=360.f;} while (h<0.f); }
        else if (h>360.f) { do {h-=360.f;} while (h>360.f); }
        const float q=l<0.5f ? l*(1.f+s) : (l+s)-(l*s);
        const float p=(l+l)-q,base=h/360.f;
        palette[i]=colour_byte(hue_channel(p,q,base+1.f/3.f));
        palette[i+1]=colour_byte(hue_channel(p,q,base));
        palette[i+2]=colour_byte(hue_channel(p,q,base-1.f/3.f));
    }
}
}

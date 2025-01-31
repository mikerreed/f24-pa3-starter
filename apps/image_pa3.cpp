/**
 *  Copyright 2015 Mike Reed
 */

#include "image.h"
#include "../include/GCanvas.h"
#include "../include/GBitmap.h"
#include "../include/GColor.h"
#include "../include/GPoint.h"
#include "../include/GRandom.h"
#include "../include/GRect.h"
#include "../include/GShader.h"
#include <string>

// deliberately take params by value, to ensure that the impl
// is making a copy, and not just taking its address.
static std::shared_ptr<GShader> make_bm_shader(GBitmap bm, GMatrix localM) {
    auto sh = GCreateBitmapShader(bm, localM);
    // just to be sure the factory didn't store pointers
    bm.reset();
    localM = GMatrix::Scale(0, 0);
    if (false) {
        printf("%p%p", &bm, &localM);
    }
    return sh;
}

class CheckerShader : public GShader {
    const GMatrix fLocalMatrix;
    const GPixel fP0, fP1;

    GMatrix fInverse;
    
public:
    CheckerShader(float scale, GPixel p0, GPixel p1)
        : fLocalMatrix(GMatrix::Scale(scale, scale))
        , fP0(p0)
        , fP1(p1)
    {}
    
    bool isOpaque() override {
        return GPixel_GetA(fP0) == 0xFF && GPixel_GetA(fP1) == 0xFF;
    }
    
    bool setContext(const GMatrix& ctm) override {
        if (auto inv = (ctm * fLocalMatrix).invert()) {
            fInverse = *inv;
            return true;
        }
        return false;
    }
    
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        const auto step = fInverse.e0();
        GPoint loc = fInverse * GPoint{x + 0.5f, y + 0.5f};
        
        const GPixel array[] = { fP0, fP1 };
        for (int i = 0; i < count; ++i) {
            row[i] = array[((int)loc.x + (int)loc.y) & 1];
            loc += step;
        }
    }
};

static void draw_checker(GCanvas* canvas) {
    GPaint paint(std::make_shared<CheckerShader>(20,
                                                 GPixel_PackARGB(0xFF, 0, 0, 0),
                                                 GPixel_PackARGB(0xFF, 0xFF, 0xFF, 0xFF)));

    canvas->clear({ 0.75, 0.75, 0.75, 1 });
    canvas->drawRect(GRect::XYWH(20, 20, 100, 100), paint);
    
    canvas->save();
    canvas->translate(130, 175);
    canvas->rotate(-gFloatPI/3);
    canvas->drawRect(GRect::XYWH(20, 20, 100, 100), paint);
    canvas->restore();
    
    canvas->save();
    canvas->translate(10, 160);
    canvas->scale(0.5, 0.5);
    canvas->drawRect(GRect::XYWH(20, 20, 200, 200), paint);
    canvas->restore();

    paint.setShader(std::make_shared<CheckerShader>(150,
                                                    GPixel_PackARGB(0x44, 0x44, 0, 0),
                                                    GPixel_PackARGB(0x44, 0, 0, 0x44)));
    canvas->drawRect(GRect::XYWH(0, 0, 300, 300), paint);
}

static void draw_poly_rotate(GCanvas* canvas) {
    const GPoint pts[] {
        { 0, 0.1f }, { -1, 5 }, { 0, 6 }, { 1, 5 },
    };
    
    canvas->translate(150, 150);
    canvas->scale(25, 25);
    
    float steps = 12;
    float r = 0;
    float b = 1;
    float step = 1 / (steps - 1);
    
    GPaint paint;
    
    for (float angle = 0; angle < 2*gFloatPI - 0.001f; angle += 2*gFloatPI/steps) {
        paint.setColor({ r, 0, b, 1 });
        canvas->save();
        canvas->rotate(angle);
        canvas->drawConvexPolygon(pts, 4, paint);
        canvas->restore();
        r += step;
        b -= step;
    }
}

static void draw_clock_bm(GCanvas* canvas) {
    GBitmap bm;
    bm.readFromFile("apps/spock.png");

    float cx = bm.width() * 0.5f;
    float cy = bm.height() * 0.5f;
    GPoint pts[] = {
        { cx, 0 }, { 0, cy }, { cx, bm.height()*1.0f }, { bm.width()*1.0f, cy },
    };

    GPaint paint(make_bm_shader(bm, GMatrix()));

    int n = 7;
    canvas->scale(0.4f, 0.4f);
    for (int i = 0; i < n; ++i) {
        float radians = i * gFloatPI * 2 / n;
        canvas->save();
        canvas->translate(cx*3, cx*3);
        canvas->rotate(radians);
        canvas->translate(cx, -cy);
        canvas->drawConvexPolygon(pts, 4, paint);
        canvas->restore();
    }
}

static void draw_bitmap(GCanvas* canvas, const GRect& r, const GBitmap& bm, GBlendMode mode) {
    GPaint paint;
    paint.setBlendMode(mode);
    
    GMatrix m = GMatrix::Translate(r.left, r.top)
              * GMatrix::Scale(r.width() / bm.width(), r.height() / bm.height());

    paint.setShader(make_bm_shader(bm, m));
    canvas->drawRect(r, paint);
}

static void draw_bitmaps_hole(GCanvas* canvas) {
    GBitmap bm0, bm1;
    
    bm0.readFromFile("apps/spock.png");
    bm1.readFromFile("apps/wheel.png"); // with permission from Alex Niculescu

    const GRect r = GRect::WH(300, 300);
    draw_bitmap(canvas, r, bm0, GBlendMode::kSrc);
    draw_bitmap(canvas, r, bm1, GBlendMode::kDstIn);
}

static void draw_mode_sample2(GCanvas* canvas, const GRect& bounds, GBlendMode mode) {
    outer_frame(canvas, bounds);

    GPaint paint;
    GPoint pts[4];

    GPixel dstp[5] = {
        GPixel_PackARGB(0, 0, 0, 0),
        GPixel_PackARGB(0x40, 0x40, 0, 0),
        GPixel_PackARGB(0x80, 0x80, 0, 0),
        GPixel_PackARGB(0xC0, 0xC0, 0, 0),
        GPixel_PackARGB(0xFF, 0xFF, 0, 0),
    };
    GBitmap dstbm(1, 5, 1*4, dstp, false);
    auto dstsh = make_bm_shader(dstbm, GMatrix::Scale(bounds.width(), bounds.height()/5.0f));

    GPixel srcp[5] = {
        GPixel_PackARGB(0, 0, 0, 0),
        GPixel_PackARGB(0x40, 0, 0, 0x40),
        GPixel_PackARGB(0x80, 0, 0, 0x80),
        GPixel_PackARGB(0xC0, 0, 0, 0xC0),
        GPixel_PackARGB(0xFF, 0, 0, 0xFF),
    };
    GBitmap srcbm(5, 1, 5*4, srcp, false);
    auto srcsh = make_bm_shader(srcbm, GMatrix::Scale(bounds.width()/5.0f, bounds.height()));

    paint.setBlendMode(GBlendMode::kSrc);
    paint.setShader(dstsh);
    canvas->drawConvexPolygon(rect_pts(bounds, pts), 4, paint);

    paint.setBlendMode(mode);
    paint.setShader(srcsh);
    canvas->drawRect(bounds, paint);
}

static void draw_all_blendmodes(GCanvas* canvas, void (*proc)(GCanvas*, const GRect&, GBlendMode)) {
    canvas->clear({1,1,1,1});
    const GRect r = GRect::WH(100, 100);

    const float W = 100;
    const float H = 100;
    const float margin = 10;
    float x = margin;
    float y = margin;
    for (int i = 0; i < 12; ++i) {
        GBlendMode mode = static_cast<GBlendMode>(i);
        canvas->save();
        canvas->translate(x, y);
        proc(canvas, r, mode);
        canvas->restore();
        if (i % 4 == 3) {
            y += H + margin;
            x = margin;
        } else {
            x += W + margin;
        }
    }
}

static void draw_bm_blendmodes(GCanvas* canvas) {
    draw_all_blendmodes(canvas, draw_mode_sample2);
}

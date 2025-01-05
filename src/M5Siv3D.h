/*
 * M5Siv3D
 * https://github.com/chobby/M5Siv3D
 * Copyright (c) 2024 chobby
 * Licensed under MIT License
 *
 * M5Siv3D - A wrapper library for M5Unified
 *
 * This library provides an OpenSiv3D-inspired API for M5Stack devices,
 * offering familiar and intuitive graphics and input handling interfaces
 * while maintaining full compatibility with M5Unified's powerful features.
 *
 * Designed to make creative coding on M5Stack devices more accessible
 * through a simplified, yet powerful programming interface.
 */

#include <M5Unified.h>
#include <base64.hpp>  // Densaugeoのライブラリ
#include <SPI.h>

// 数学ユーティリティを格納する名前空間
namespace Math
{
    template <typename T>
    struct Vec2
    {
        T x, y;

        Vec2(T _x = 0, T _y = 0) : x(_x), y(_y) {}

        // 基本的な演算子のオーバーロード
        Vec2 operator+(const Vec2 &other) const { return Vec2(x + other.x, y + other.y); }
        Vec2 operator-(const Vec2 &other) const { return Vec2(x - other.x, y - other.y); }
        Vec2 operator*(T scalar) const { return Vec2(x * scalar, y * scalar); }
        Vec2 operator/(T scalar) const { return Vec2(x / scalar, y / scalar); }

        // ドット積
        T dot(const Vec2 &other) const { return x * other.x + y * other.y; }

        // 長さ
        T length() const { return std::sqrt(lengthSquared()); }
        T lengthSquared() const { return x * x + y * y; }

        // 正規化
        Vec2 normalized() const
        {
            T len = length();
            return len ? (*this / len) : Vec2();
        }
    };

    template <typename T>
    struct Vec3
    {
        T x, y, z;

        Vec3(T _x = 0, T _y = 0, T _z = 0) : x(_x), y(_y), z(_z) {}

        // 基本的な演算子のオーバーロード
        Vec3 operator+(const Vec3 &other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
        Vec3 operator-(const Vec3 &other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
        Vec3 operator*(T scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
        Vec3 operator/(T scalar) const { return Vec3(x / scalar, y / scalar, z / scalar); }

        // ドット積
        T dot(const Vec3 &other) const { return x * other.x + y * other.y + z * other.z; }

        // クロス積
        Vec3 cross(const Vec3 &other) const
        {
            return Vec3(
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x);
        }

        // 長さ
        T length() const { return std::sqrt(lengthSquared()); }
        T lengthSquared() const { return x * x + y * y + z * z; }

        // 正規化
        Vec3 normalized() const
        {
            T len = length();
            return len ? (*this / len) : Vec3();
        }
    };

    // 便利な型エイリアス
    using Vec2f = Vec2<float>;
    using Vec2d = Vec2<double>;
    using Vec2i = Vec2<int32_t>;
    using Vec3f = Vec3<float>;
    using Vec3d = Vec3<double>;
    using Vec3i = Vec3<int32_t>;
}

// Color構造体を先に定義
struct Color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;

    Color(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}

    // RGB565形式の整数からColorを生成するコンストラクタ
    Color(uint16_t rgb565)
    {
        r = (rgb565 >> 8) & 0xF8; // 赤色成分を抽出し、5ビットから8ビットに拡張
        g = (rgb565 >> 3) & 0xFC; // 緑色成分を抽出し、6ビットから8ビットに拡張
        b = (rgb565 << 3) & 0xF8; // 青色成分を抽出し、5ビットから8ビットに拡張

        r |= r >> 5; // 下位ビットを埋める
        g |= g >> 6;
        b |= b >> 5;
    }

    // RGB888形式の整数からColorを生成するコンストラクタ
    void setFromRGB888(uint32_t rgb888)
    {
        r = (rgb888 >> 16) & 0xFF; // 赤色成分
        g = (rgb888 >> 8) & 0xFF;  // 緑色成分
        b = rgb888 & 0xFF;         // 青色成分
    }

    // RGB565形式への変換
    uint16_t toRGB565() const
    {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

    // OpenSiv3D風の色操作メソッドを追加
    Color lerp(const Color &other, float t) const
    {
        return Color(
            r + (other.r - r) * t,
            g + (other.g - g) * t,
            b + (other.b - b) * t);
    }

    Color operator+(const Color &other) const
    {
        return Color(
            std::min(255, int(r) + int(other.r)),
            std::min(255, int(g) + int(other.g)),
            std::min(255, int(b) + int(other.b)));
    }

    // HSVからRGBを生成する静的メソッド
    static Color FromHSV(float h, float s, float v)
    {
        h = fmod(h, 360.0f);
        s = std::min(1.0f, std::max(0.0f, s));
        v = std::min(1.0f, std::max(0.0f, v));

        float c = v * s;
        float x = c * (1 - std::abs(fmod(h / 60.0f, 2) - 1));
        float m = v - c;

        float r = 0, g = 0, b = 0;

        if (h >= 0 && h < 60)
        {
            r = c;
            g = x;
            b = 0;
        }
        else if (h >= 60 && h < 120)
        {
            r = x;
            g = c;
            b = 0;
        }
        else if (h >= 120 && h < 180)
        {
            r = 0;
            g = c;
            b = x;
        }
        else if (h >= 180 && h < 240)
        {
            r = 0;
            g = x;
            b = c;
        }
        else if (h >= 240 && h < 300)
        {
            r = x;
            g = 0;
            b = c;
        }
        else
        {
            r = c;
            g = 0;
            b = x;
        }

        return Color(
            uint8_t((r + m) * 255),
            uint8_t((g + m) * 255),
            uint8_t((b + m) * 255));
    }

    // RGBからHSVに変換するメソッド
    void toHSV(float &h, float &s, float &v) const
    {
        float r_norm = r / 255.0f;
        float g_norm = g / 255.0f;
        float b_norm = b / 255.0f;

        float cmax = std::max({r_norm, g_norm, b_norm});
        float cmin = std::min({r_norm, g_norm, b_norm});
        float diff = cmax - cmin;

        // Hue calculation
        if (diff == 0)
            h = 0;
        else if (cmax == r_norm)
            h = fmod(60 * ((g_norm - b_norm) / diff) + 360, 360);
        else if (cmax == g_norm)
            h = 60 * ((b_norm - r_norm) / diff) + 120;
        else if (cmax == b_norm)
            h = 60 * ((r_norm - g_norm) / diff) + 240;

        // Saturation calculation
        s = (cmax == 0) ? 0 : (diff / cmax);

        // Value calculation
        v = cmax;
    }
};

// 色定数の追加
namespace Palette
{
    const Color Black(0, 0, 0);
    const Color White(255, 255, 255);
    const Color Red(255, 0, 0);
    const Color Green(0, 255, 0);
    const Color Blue(0, 0, 255);
    const Color Yellow(255, 255, 0);
    const Color Magenta(255, 0, 255);
    const Color Cyan(0, 255, 255);
}

namespace Input
{
    class InputManager
    {
    public:
        static InputManager &getInstance()
        {
            static InputManager instance;
            return instance;
        }

        void update()
        {
            // ボタンの状態を更新
            M5.update();
        }

    private:
        InputManager() = default;
    };

    class ButtonState
    {
    private:
        m5::Button_Class *m_button;

    public:
        ButtonState(m5::Button_Class *btn) : m_button(btn) {}

        bool down() const { return m_button->isPressed(); }
        bool up() const { return m_button->isReleased(); }
        bool pressed() const { return m_button->wasPressed(); }
        bool released() const { return m_button->wasReleased(); }
        bool pressedDuration(uint32_t ms) const { return m_button->pressedFor(ms); }
        bool releasedDuration(uint32_t ms) const { return m_button->releasedFor(ms); }
    };

    // グローバルなボタンステート
    inline ButtonState ButtonA{&M5.BtnA};
    inline ButtonState ButtonB{&M5.BtnB};
    inline ButtonState ButtonC{&M5.BtnC};

    class IMU
    {
    public:
        static IMU &getInstance()
        {
            static IMU instance;
            return instance;
        }

        // 加速度を取得 (G)
        Math::Vec3f getAccel() const
        {
            auto data = M5.Imu.getImuData();
            return Math::Vec3f(data.accel.x, data.accel.y, data.accel.z);
        }

        // 角速度を取得 (deg/s)
        Math::Vec3f getGyro() const
        {
            auto data = M5.Imu.getImuData();
            return Math::Vec3f(data.gyro.x, data.gyro.y, data.gyro.z);
        }

        // 地磁気を取得 (μT)
        Math::Vec3f getMag() const
        {
            auto data = M5.Imu.getImuData();
            return Math::Vec3f(data.mag.x, data.mag.y, data.mag.z);
        }

        // ... rest of IMU implementation ...
    };

    // グローバルなIMUインスタンス
    inline IMU &IMU = IMU::getInstance();
}

class System
{
public:
    // シングルトンインスタンスの取得
    static System &getInstance()
    {
        static System instance;
        return instance;
    }

    // システムの初期化
    void begin()
    {
        auto cfg = M5.config();
        M5.begin(cfg);

        // キャンバスを画面のサイズで初期化
        canvas.createSprite(M5.Display.width(), M5.Display.height());
        canvas.setTextSize(2);
        lastDrawTime = millis();
    }

    // 描画の開始
    void beginDraw()
    {
        canvas.fillSprite(m_backgroundColor.toRGB565());
    }

    // 描画の終了と画面更新
    void endDraw()
    {
        canvas.pushSprite(0, 0);
    }

    void setBackgroundColor(const Color &color)
    {
        m_backgroundColor = color;
    }

    // メインループの更新処理
    bool Update()
    {
        uint32_t currentTime = millis();
        uint32_t elapsedTime = currentTime - lastDrawTime;

        if (elapsedTime >= FRAME_INTERVAL)
        {
            // 描画終了
            endDraw();

            lastDrawTime = currentTime;

            // 残り時間をスリープ
            int32_t remaining = FRAME_INTERVAL - (millis() - currentTime);
            if (remaining > 0)
            {
                M5.delay(remaining);
            }

            Input::InputManager::getInstance().update(); // InputManagerで更新
            beginDraw();
        }
        return true; // ループ継続
    }

    // キャンバスへのアクセス
    M5Canvas &getCanvas()
    {
        return canvas;
    }

    // 画面の幅と高さの取得
    int getWidth() const { return M5.Display.width(); }
    int getHeight() const { return M5.Display.height(); }

    // 時間管理関連のメソッドを追加
    float getDeltaTime() const { return m_deltaTime; }
    float getFPS() const { return 1000.0f / m_averageFrameTime; }
    uint64_t getFrameCount() const { return m_frameCount; }

    // 経過時間を秒単位で取得
    double getTime() const
    {
        return millis() / 1000.0;
    }

private:
    System()
    {

        begin();
    } // プライベートコンストラクタ

    // システム変数
    static constexpr int FRAME_INTERVAL = 16; // 約60FPS
    uint32_t lastDrawTime = 0;
    M5Canvas canvas{&M5.Display};

    // コピー禁止
    System(const System &) = delete;
    System &operator=(const System &) = delete;

    Color m_backgroundColor = Palette::Black;

    // 時間管理用メンバ変数
    float m_deltaTime = 0.0f;
    float m_averageFrameTime = FRAME_INTERVAL;
    uint64_t m_frameCount = 0;
    uint32_t m_previousTime = 0;

    // Update内で呼び出す時間更新処理
    void updateTime()
    {
        uint32_t currentTime = millis();
        m_deltaTime = (currentTime - m_previousTime) / 1000.0f;

        // 移動平均でFPSを計算
        m_averageFrameTime = m_averageFrameTime * 0.9f + (currentTime - m_previousTime) * 0.1f;

        m_previousTime = currentTime;
        m_frameCount++;
    }
};

// Print関数を名前空間内に移動
void Print(const std::string &text)
{
    System::getInstance().getCanvas().print(text.c_str());
}

void Println(const std::string &text)
{
    System::getInstance().getCanvas().println(text.c_str());
}

struct Circle
{
    int32_t m_x;
    int32_t m_y;
    int32_t m_r;

    Circle(int32_t x, int32_t y, int32_t r) : m_x(x), m_y(y), m_r(r)
    {
    }

    void draw(const Color &color = Color(0, 0, 0))
    {
        System::getInstance().getCanvas().fillCircle(m_x, m_y, m_r, color.toRGB565());
    }

    void drawFrame(const Color &color = Color(0, 0, 0))
    {
        System::getInstance().getCanvas().drawCircle(m_x, m_y, m_r, color.toRGB565());
    }

    void drawArc(int32_t thickness, int32_t startAngle, int32_t endAngle, const Color &color = Color(0, 0, 0))
    {
        System::getInstance().getCanvas().drawArc(m_x, m_y, m_r, thickness, startAngle, endAngle, color.toRGB565());
    }

    void fillArc(int32_t thickness, int32_t startAngle, int32_t endAngle, const Color &color = Color(0, 0, 0))
    {
        System::getInstance().getCanvas().fillArc(m_x, m_y, m_r, thickness, startAngle, endAngle, color.toRGB565());
    }
};

struct Rect
{
    int32_t m_x;
    int32_t m_y;
    int32_t m_width;
    int32_t m_height;

    Rect(int32_t x, int32_t y, int32_t width, int32_t height) : m_x(x), m_y(y), m_width(width), m_height(height)
    {
    }

    void draw(const Color &color = Color(0, 0, 0))
    {
        System::getInstance().getCanvas().fillRect(m_x, m_y, m_width, m_height, color.toRGB565());
    }

    void drawFrame(const Color &color = Color(0, 0, 0))
    {
        System::getInstance().getCanvas().drawRect(m_x, m_y, m_width, m_height, color.toRGB565());
    }
};

struct Triangle
{
    int32_t m_x1, m_y1, m_x2, m_y2, m_x3, m_y3;

    Triangle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3) : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2), m_x3(x3), m_y3(y3)
    {
    }

    void draw(const Color &color = Color(0, 0, 0))
    {
        System::getInstance().getCanvas().fillTriangle(m_x1, m_y1, m_x2, m_y2, m_x3, m_y3, color.toRGB565());
    }

    void drawFrame(const Color &color = Color(0, 0, 0))
    {
        System::getInstance().getCanvas().drawTriangle(m_x1, m_y1, m_x2, m_y2, m_x3, m_y3, color.toRGB565());
    }
};

struct Line
{
    int32_t m_x1, m_y1, m_x2, m_y2;

    Line(int32_t x1, int32_t y1, int32_t x2, int32_t y2) : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2)
    {
    }

    void draw(const Color &color = Color(0, 0, 0))
    {
        System::getInstance().getCanvas().drawLine(m_x1, m_y1, m_x2, m_y2, color.toRGB565());
    }
};

// Font構造体の拡張
struct Font
{
    // TextAlignをFont構造体の外に移動
    enum class TextAlign
    {
        Left,
        Center,
        Right
    };

    TextAlign align; // これで TextAlign が認識される

    const lgfx::IFont *m_fontPtr;
    float m_size = 1.0f; // サイズ指定用の変数を追加

    // コンストラクタを修正
    Font(const lgfx::IFont &font = fonts::Font0)
        : m_fontPtr(&font), align(TextAlign::Left) {}

    Font &setAlign(TextAlign a)
    {
        align = a;
        return *this;
    }

    // サイズ設定用のメソッドを追加
    Font &setSize(float size)
    {
        m_size = size;
        return *this;
    }

    // 描画メソッドを修正
    void draw(const String &text, int x, int y, const Color &color = Palette::White)
    {
        auto &canvas = System::getInstance().getCanvas();
        canvas.setTextColor(color.toRGB565());
        canvas.setFont(m_fontPtr);
        canvas.setTextSize(m_size); // サイズを設定

        // テキストアライメントの処理
        int actualX = x;
        if (align != TextAlign::Left)
        {
            int w = textWidth(text);
            if (align == TextAlign::Center)
                actualX = x - (w / 2);
            else if (align == TextAlign::Right)
                actualX = x - w;
        }

        canvas.drawString(text, actualX, y);
    }

    // 描画位置を指定するための構造体
    struct Pos
    {
        int x, y;
        Pos(int _x, int _y) : x(_x), y(_y) {}
    };

    // 演算子オーバーロードで簡潔な描画
    void operator()(const String &text, const Pos &pos, const Color &color = Palette::White)
    {
        draw(text, pos.x, pos.y, color);
    }

    // テキストの幅を取得するメソッドを修正
    int textWidth(const String &text) const
    {
        return System::getInstance().getCanvas().textWidth(text) * m_size;
    }

    // テキストの高さを取得するメソッドを修正
    int textHeight() const
    {
        return System::getInstance().getCanvas().fontHeight() * m_size;
    }

    // 描画領域を取得
    Rect region(const String &text, int x, int y) const
    {
        return Rect(x, y, textWidth(text), textHeight());
    }
};

class Image {
private:
    M5Canvas* m_canvas;
    bool m_valid = false;
    int32_t m_width = 0;
    int32_t m_height = 0;

public:
    Image() : m_canvas(nullptr) {
        // キャンバスを作成
        m_canvas = new M5Canvas(&M5.Display);
        if (m_canvas) {
            m_canvas->setColorDepth(16);  // 16bitカラーモードを設定
        }
    }
    
    bool loadBase64(const char* base64Data) {
        if (!m_canvas) {
            Serial.println("Canvas not initialized");
            return false;
        }

        // 既存のスプライトを削除
        m_canvas->deleteSprite();
        m_valid = false;
        
        // Base64デコード
        size_t decodedLen = decode_base64_length((unsigned char*)base64Data);
        if (decodedLen == 0) {
            Serial.println("Base64 decode length is 0");
            return false;
        }
        
        uint8_t* decodedData = new uint8_t[decodedLen];
        size_t actualLen = decode_base64((unsigned char*)base64Data, decodedData);
        if (actualLen == 0) {
            Serial.println("Base64 decode failed");
            delete[] decodedData;
            return false;
        }

        // PNGヘッダーからサイズを読み取る (IHDRチャンク)
        if (actualLen < 24 || decodedData[0] != 0x89 || decodedData[1] != 'P' || decodedData[2] != 'N' || decodedData[3] != 'G') {
            Serial.println("Invalid PNG format");
            delete[] decodedData;
            return false;
        }

        // 幅と高さを取得 (ビッグエンディアン)
        m_width = (decodedData[16] << 24) | (decodedData[17] << 16) | (decodedData[18] << 8) | decodedData[19];
        m_height = (decodedData[20] << 24) | (decodedData[21] << 16) | (decodedData[22] << 8) | decodedData[23];
        
        Serial.printf("Image dimensions from PNG header: %dx%d\n", m_width, m_height);

        // 実際のサイズでスプライトを作成
        if (!m_canvas->createSprite(m_width, m_height)) {
            Serial.println("Failed to create sprite");
            delete[] decodedData;
            return false;
        }

        
        // PNG画像を描画
        if (!m_canvas->drawPng(decodedData, actualLen, 0, 0)) {
            Serial.println("Failed to draw PNG");
            delete[] decodedData;
            return false;
        }

        delete[] decodedData;
        m_valid = true;
        
        Serial.println("Image loaded successfully");
        return true;
    }

    bool create(int32_t width, int32_t height) {
        if (!m_canvas) {
            Serial.println("Canvas not initialized");
            return false;
        }

        // 既存のスプライトを削除
        m_canvas->deleteSprite();
        m_valid = false;

        // 新しいサイズでスプライトを作成
        if (!m_canvas->createSprite(width, height)) {
            Serial.println("Failed to create sprite");
            return false;
        }

        m_width = width;
        m_height = height;
        m_canvas->fillScreen(TFT_BLACK);
        m_valid = true;
        return true;
    }

    void draw(int32_t x, int32_t y) const {
        if (m_valid && m_canvas) {
            m_canvas->pushSprite(&System::getInstance().getCanvas(), x, y);
        }
    }

    void draw(int32_t x, int32_t y, uint32_t transparentColor) const {
        if (m_valid && m_canvas) {
            m_canvas->pushSprite(&System::getInstance().getCanvas(), x, y, transparentColor);
        }
    }

    int32_t width() const { return m_width; }
    int32_t height() const { return m_height; }
    bool isEmpty() const { return !m_valid || !m_canvas; }
    Math::Vec2i size() const { return Math::Vec2i(m_width, m_height); }


    ~Image() {
        if (m_canvas) {
            m_canvas->deleteSprite();
            delete m_canvas;
            m_canvas = nullptr;
        }
    }
};

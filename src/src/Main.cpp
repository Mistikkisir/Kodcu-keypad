#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <array>
#include <map>
#include <string>

using namespace geode::prelude;

// Tuş konumları, resources/keypad.png üzerinde 0..1 arası oran olarak:
// {soldan, yukarıdan, genişlik, yükseklik}
struct KeyRect {
    float u, v, w, h;
};

static constexpr std::array<KeyRect, 3> KEYS = {{
    {0.038f, 0.116f, 0.210f, 0.590f}, // tuş 1 (sol)
    {0.252f, 0.106f, 0.198f, 0.609f}, // tuş 2 (orta)
    {0.660f, 0.120f, 0.207f, 0.605f}, // tuş 3 (sağ)
}};

class KeypadOverlay : public CCLayer {
protected:
    CCSprite* m_bg = nullptr;
    std::array<CCLayerColor*, 3> m_glow = {nullptr, nullptr, nullptr};
    std::array<std::string, 3> m_held;          // tuş basılıyken hangi aksiyon çalışıyor
    std::map<CCTouch*, int> m_touchToKey;       // hangi parmak hangi tuşta

    bool init() override {
        if (!CCLayer::init()) return false;

        m_bg = CCSprite::create("keypad.png"_spr);
        if (!m_bg) return false;
        this->addChild(m_bg);

        auto size = m_bg->getContentSize();
        for (int i = 0; i < 3; i++) {
            auto r = KEYS[i];
            auto glow = CCLayerColor::create(ccc4(255, 255, 255, 110), r.w * size.width, r.h * size.height);
            glow->setPosition({r.u * size.width, (1.f - r.v - r.h) * size.height});
            glow->setVisible(false);
            m_bg->addChild(glow, 1);
            m_glow[i] = glow;
        }

        this->applySettings();
        this->setTouchEnabled(true);
        return true;
    }

    void applySettings() {
        auto win = CCDirector::sharedDirector()->getWinSize();
        auto mod = Mod::get();
        float sizePct = static_cast<float>(mod->getSettingValue<double>("size"));
        float px = static_cast<float>(mod->getSettingValue<double>("pos-x"));
        float py = static_cast<float>(mod->getSettingValue<double>("pos-y"));
        int opacity = static_cast<int>(mod->getSettingValue<int64_t>("opacity"));

        float targetWidth = win.width * sizePct / 100.f;
        m_bg->setScale(targetWidth / m_bg->getContentSize().width);
        m_bg->setPosition({win.width * px / 100.f, win.height * py / 100.f});
        m_bg->setOpacity(static_cast<GLubyte>(opacity));
    }

    void registerWithTouchDispatcher() override {
        CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -600, true);
    }

    // Duraklatma menüsü veya bir uyarı açıksa dokunmayı yutma
    static bool blockedByOtherUI() {
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        if (!scene) return true;
        if (scene->getChildByType<PauseLayer>(0)) return true;
        if (scene->getChildByType<FLAlertLayer>(0)) return true;
        return false;
    }

    int keyAt(CCTouch* touch) {
        auto local = m_bg->convertToNodeSpace(touch->getLocation());
        auto size = m_bg->getContentSize();
        for (int i = 0; i < 3; i++) {
            auto r = KEYS[i];
            float x = r.u * size.width;
            float y = (1.f - r.v - r.h) * size.height;
            if (local.x >= x && local.x <= x + r.w * size.width &&
                local.y >= y && local.y <= y + r.h * size.height) {
                return i;
            }
        }
        return -1;
    }

    void press(int key) {
        auto pl = PlayLayer::get();
        if (!pl) return;

        auto action = Mod::get()->getSettingValue<std::string>(fmt::format("key{}-action", key + 1));
        m_held[key] = action;
        m_glow[key]->setVisible(true);

        if (action == "p1-jump" || action == "both-jump") pl->handleButton(true, 1, true);
        if (action == "p2-jump" || action == "both-jump") pl->handleButton(true, 1, false);
        if (action == "pause") pl->pauseGame(true);
    }

    void release(int key) {
        m_glow[key]->setVisible(false);
        auto action = m_held[key];
        m_held[key].clear();

        auto pl = PlayLayer::get();
        if (!pl) return;
        if (action == "p1-jump" || action == "both-jump") pl->handleButton(false, 1, true);
        if (action == "p2-jump" || action == "both-jump") pl->handleButton(false, 1, false);
    }

public:
    static KeypadOverlay* create() {
        auto ret = new KeypadOverlay();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        if (blockedByOtherUI()) return false;
        int key = this->keyAt(touch);
        if (key < 0) return false; // tuş dışı dokunmalar normal oyuna gitsin
        m_touchToKey[touch] = key;
        this->press(key);
        return true;
    }

    void ccTouchEnded(CCTouch* touch, CCEvent*) override {
        auto it = m_touchToKey.find(touch);
        if (it == m_touchToKey.end()) return;
        this->release(it->second);
        m_touchToKey.erase(it);
    }

    void ccTouchCancelled(CCTouch* touch, CCEvent* event) override {
        this->ccTouchEnded(touch, event);
    }

    void onExit() override {
        for (int i = 0; i < 3; i++) {
            if (!m_held[i].empty()) this->release(i);
        }
        m_touchToKey.clear();
        CCLayer::onExit();
    }
};

class $modify(KeypadPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        if (Mod::get()->getSettingValue<bool>("enabled")) {
            if (auto overlay = KeypadOverlay::create()) {
                this->addChild(overlay, 9999);
            }
        }
        return true;
    }
};

cmake_minimum_required(VERSION 3.22)
project(HarmonicGlitchPlugin VERSION 0.1.0)

# C++17規格を指定
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCEサブモジュールを読み込み
add_subdirectory(JUCE)

# プラグインターゲットの定義
juce_add_plugin(HarmonicGlitchPlugin
    COMPANY_NAME "MyAudioLab"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Hglt
    PLUGIN_CODE Hglp
    FORMATS VST3 Standalone # 必要なフォーマットを指定
    PRODUCT_NAME "HarmonicGlitch"
)

# ソースファイルの指定
target_sources(HarmonicGlitchPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

# 必要なJUCEモジュールをリンク
target_link_libraries(HarmonicGlitchPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
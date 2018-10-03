/*
  ==============================================================================

    SynthParameters.h
    Created: 3 Oct 2018 4:34:13pm
    Author:  COx2

  ==============================================================================
*/

#pragma once

// ①JUCEライブラリのヘッダをインクルードする。
#include "../JuceLibraryCode/JuceHeader.h"

// ②クラス宣言。複数のパラメータをまとめるクラス群の基底クラス。
class SynthParametersBase
{
public:
	// デストラクタ
	virtual ~SynthParametersBase() {};

	// ③継承クラス側で実装を必須とする関数を純粋仮想関数として宣言する。
	virtual void addAllParameters(AudioProcessor& processor) = 0;
	virtual void saveParameters(XmlElement& xml) = 0;
	virtual void loadParameters(XmlElement& xml) = 0;
};

// ④クラス宣言。OSC MIXモジュールを操作するパラメータ群を管理するクラス。
class AmpEGParameters : public SynthParametersBase
{
public:
	// ⑤各波形の音量レベルを管理するパラメータのポインタ変数。
	AudioParameterFloat* Attack;
	AudioParameterFloat* Decay;
	AudioParameterFloat* Sustain;
	AudioParameterFloat* Release;

	// ⑥引数付きコンストラクタ。PluginProcessor.h/cpp側で保持するパラメータのポインタ変数を受け取る。
	AmpEGParameters(AudioParameterFloat* attack,
					AudioParameterFloat* decay,
					AudioParameterFloat* sustain,
					AudioParameterFloat* release);


	// ⑦基底クラスで宣言されている純粋仮想関数をオーバーライドして実装する。
	virtual void addAllParameters(AudioProcessor& processor) override;
	virtual void saveParameters(XmlElement& xml) override;
	virtual void loadParameters(XmlElement& xml) override;

private:
	// 引数無しコントラクタをprivate領域に置くことで、外から呼び出せないようにする。
	AmpEGParameters() {};
};

#include "plugin.hpp"


struct Vareo : Module {
	enum ParamId {
		REC_BUTTON_PARAM,
		JUMP_BUTTON_PARAM,
		SPEED_KNOB_PARAM,
		SPEED_ATTENUVERTER_PARAM,
		DELAY_KNOB_PARAM,
		JUMP_DIR_SWITCH_PARAM,
		BLEND_MODE_SWITCH_PARAM,
		BLEND_KNOB_PARAM,
		JUMP_MODE_SWITCH_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		JUMP_INPUT_INPUT,
		L_AUDIO_INPUT_INPUT,
		SPEED_INPUT_INPUT,
		REC_INPUT_INPUT,
		R_AUDIO_INPUT_INPUT,
		DELAY_INPUT_INPUT,
		BLEND_INPUT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		END_OUTPUT_OUTPUT,
		L_AUDIO_OUTPUT_OUTPUT,
		R_AUDIO_OUTPUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		REC_LED_LIGHT,
		JUMP_TOLERANCE_LED_LIGHT,
		END_LED_LIGHT,
		LIGHTS_LEN
	};

	Vareo() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configButton(REC_BUTTON_PARAM, "Record Button");
		configButton(JUMP_BUTTON_PARAM, "Jump Button");
		configParam(SPEED_KNOB_PARAM, -1.f, 1.f, 0.f, "Speed");
		configParam(SPEED_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Speed CV Attenuverter");
		configParam(DELAY_KNOB_PARAM, 0.f, 1.f, 0.f, "Read Delay");
		configSwitch(JUMP_DIR_SWITCH_PARAM, 0.f, 1.f, 0.f, "Jump Direction", {"Forward", "Backward"});
		configSwitch(BLEND_MODE_SWITCH_PARAM, 0.f, 1.f, 0.f, "Blend Mode", {"Add", "Average"});
		configParam(BLEND_KNOB_PARAM, 0.f, 1.f, 0.f, "Write Blend");
		configSwitch(JUMP_MODE_SWITCH_PARAM, 0.f, 1.f, 0.f, "Jump Mode", {"Start of Recording", "Start of Tape"});
		configInput(BLEND_INPUT_INPUT, "Write Blend CV Input");
		configInput(DELAY_INPUT_INPUT, "Read Delay CV Input");
		configInput(R_AUDIO_INPUT_INPUT, "Right Audio Input");
		configInput(REC_INPUT_INPUT, "Record Toggle Gate Input");
		configInput(SPEED_INPUT_INPUT, "Speed CV Input");
		configInput(L_AUDIO_INPUT_INPUT, "Left Audio Input");
		configInput(JUMP_INPUT_INPUT, "Jump Trigger Input");
		configOutput(R_AUDIO_OUTPUT_OUTPUT, "Right Audio Output");
		configOutput(L_AUDIO_OUTPUT_OUTPUT, "Left Audio Output");
		configOutput(END_OUTPUT_OUTPUT, "End Of Recording Output");
		configBypass(L_AUDIO_INPUT_INPUT, L_AUDIO_OUTPUT_OUTPUT);
		configBypass(R_AUDIO_INPUT_INPUT, R_AUDIO_OUTPUT_OUTPUT);
	}

	float phase = 0.f;
	float blinkPhase = 0.f;

	void process(const ProcessArgs& args) override {
		// Compute the frequency from the pitch parameter and input
		float pitch = params[SPEED_KNOB_PARAM].getValue();
		float modAmt = params[SPEED_ATTENUVERTER_PARAM].getValue();
		pitch += modAmt * inputs[SPEED_INPUT_INPUT].getVoltage();
		// The default frequency is C4 = 261.6256f
		float freq = dsp::FREQ_C4 * std::pow(2.f, pitch);

		// Accumulate the phase
		phase += freq * args.sampleTime;
		if (phase >= 1.f)
			phase -= 1.f;

		// Compute the sine output
		float sine = std::sin(2.f * M_PI * phase);
		// Audio signals are typically +/-5V
		// https://vcvrack.com/manual/VoltageStandards
		outputs[R_AUDIO_OUTPUT_OUTPUT].setVoltage(5.f * sine);

		// Blink light at 1Hz
		blinkPhase += args.sampleTime;
		if (blinkPhase >= 1.f)
			blinkPhase -= 1.f;
		lights[END_LED_LIGHT].setBrightness(blinkPhase < 0.5f ? 1.f : 0.f);
	}
};


struct VareoWidget : ModuleWidget {
	VareoWidget(Vareo* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/vareo.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<VCVLatch>(mm2px(Vec(4.471, 16.529)), module, Vareo::REC_BUTTON_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(36.169, 16.529)), module, Vareo::JUMP_BUTTON_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(20.32, 26.576)), module, Vareo::SPEED_KNOB_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(9.765, 46.328)), module, Vareo::SPEED_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(29.173, 50.779)), module, Vareo::DELAY_KNOB_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(6.503, 61.378)), module, Vareo::JUMP_DIR_SWITCH_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(34.646, 73.403)), module, Vareo::BLEND_MODE_SWITCH_PARAM));
		addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(20.32, 74.117)), module, Vareo::BLEND_KNOB_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(6.503, 83.484)), module, Vareo::JUMP_MODE_SWITCH_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.349, 91.75)), module, Vareo::JUMP_INPUT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.762, 101.75)), module, Vareo::L_AUDIO_INPUT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.291, 101.75)), module, Vareo::SPEED_INPUT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.349, 101.75)), module, Vareo::REC_INPUT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.762, 111.75)), module, Vareo::R_AUDIO_INPUT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.291, 111.75)), module, Vareo::DELAY_INPUT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.349, 111.75)), module, Vareo::BLEND_INPUT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(34.878, 91.75)), module, Vareo::END_OUTPUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(34.878, 101.75)), module, Vareo::L_AUDIO_OUTPUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(34.878, 111.75)), module, Vareo::R_AUDIO_OUTPUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(4.471, 22.507)), module, Vareo::REC_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(36.169, 22.507)), module, Vareo::JUMP_TOLERANCE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(34.878, 84.332)), module, Vareo::END_LED_LIGHT));
	}
};


Model* modelVareo = createModel<Vareo, VareoWidget>("vareo");

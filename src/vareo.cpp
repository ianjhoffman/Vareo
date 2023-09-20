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
		BLEND_INPUT_INPUT,
		DELAY_INPUT_INPUT,
		R_AUDIO_INPUT_INPUT,
		REC_INPUT_INPUT,
		SPEED_INPUT_INPUT,
		L_AUDIO_INPUT_INPUT,
		JUMP_INPUT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		R_AUDIO_OUTPUT_OUTPUT,
		L_AUDIO_OUTPUT_OUTPUT,
		END_OUTPUT_OUTPUT,
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
		configParam(REC_BUTTON_PARAM, 0.f, 1.f, 0.f, "");
		configParam(JUMP_BUTTON_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SPEED_KNOB_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SPEED_ATTENUVERTER_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DELAY_KNOB_PARAM, 0.f, 1.f, 0.f, "");
		configParam(JUMP_DIR_SWITCH_PARAM, 0.f, 1.f, 0.f, "");
		configParam(BLEND_MODE_SWITCH_PARAM, 0.f, 1.f, 0.f, "");
		configParam(BLEND_KNOB_PARAM, 0.f, 1.f, 0.f, "");
		configParam(JUMP_MODE_SWITCH_PARAM, 0.f, 1.f, 0.f, "");
		configInput(BLEND_INPUT_INPUT, "");
		configInput(DELAY_INPUT_INPUT, "");
		configInput(R_AUDIO_INPUT_INPUT, "");
		configInput(REC_INPUT_INPUT, "");
		configInput(SPEED_INPUT_INPUT, "");
		configInput(L_AUDIO_INPUT_INPUT, "");
		configInput(JUMP_INPUT_INPUT, "");
		configOutput(R_AUDIO_OUTPUT_OUTPUT, "");
		configOutput(L_AUDIO_OUTPUT_OUTPUT, "");
		configOutput(END_OUTPUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
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

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(5.0, 14.942)), module, Vareo::REC_BUTTON_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.64, 14.942)), module, Vareo::JUMP_BUTTON_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.32, 28.163)), module, Vareo::SPEED_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(9.765, 47.896)), module, Vareo::SPEED_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(29.173, 51.837)), module, Vareo::DELAY_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(5.974, 62.966)), module, Vareo::JUMP_DIR_SWITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(34.895, 73.403)), module, Vareo::BLEND_MODE_SWITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.32, 74.117)), module, Vareo::BLEND_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(5.974, 82.955)), module, Vareo::JUMP_MODE_SWITCH_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-24.82, -111.75)), module, Vareo::BLEND_INPUT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-15.82, -111.75)), module, Vareo::DELAY_INPUT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-6.82, -111.75)), module, Vareo::R_AUDIO_INPUT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-24.82, -101.75)), module, Vareo::REC_INPUT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-15.82, -101.75)), module, Vareo::SPEED_INPUT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-6.82, -101.75)), module, Vareo::L_AUDIO_INPUT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-24.82, -91.75)), module, Vareo::JUMP_INPUT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(-33.82, -111.75)), module, Vareo::R_AUDIO_OUTPUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(-33.82, -101.75)), module, Vareo::L_AUDIO_OUTPUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(-33.82, -91.75)), module, Vareo::END_OUTPUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(11.0, 14.942)), module, Vareo::REC_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(29.64, 14.942)), module, Vareo::JUMP_TOLERANCE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(33.82, 84.332)), module, Vareo::END_LED_LIGHT));
	}
};


Model* modelVareo = createModel<Vareo, VareoWidget>("vareo");
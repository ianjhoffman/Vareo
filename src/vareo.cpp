#include "plugin.hpp"
#include <dsp/digital.hpp>

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
		configParam(SPEED_KNOB_PARAM, -5.f, 5.f, 0.f, "Speed");
		configParam(SPEED_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Speed CV Attenuverter");
		configParam(DELAY_KNOB_PARAM, 0.f, 5.f, 0.f, "Read Delay");
		configSwitch(JUMP_DIR_SWITCH_PARAM, 0.f, 1.f, 0.f, "Jump Direction", {"Forward", "Backward"});
		configSwitch(BLEND_MODE_SWITCH_PARAM, 0.f, 1.f, 0.f, "Blend Mode", {"Add", "Average"});
		configParam(BLEND_KNOB_PARAM, -5.f, 5.f, 0.f, "Write Blend");
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

		lightDivider.setDivision(32);
	}

	// Input handling/state
	bool recording = false;
	dsp::BooleanTrigger recButtonTrigger;
	dsp::BooleanTrigger jumpButtonTrigger;
	dsp::SchmittTrigger recTrigger;
	dsp::SchmittTrigger jumpTrigger;
	dsp::ClockDivider lightDivider;
	float testClockPhase = 0.f;

	// Audio handling/state
	constexpr static size_t HISTORY_SIZE = 1 << 21;
	dsp::DoubleRingBuffer<float, HISTORY_SIZE> lAudioBuffer;
	dsp::DoubleRingBuffer<float, HISTORY_SIZE> rAudioBuffer;
	dsp::DoubleRingBuffer<float, 16> lOutBuffer;
	dsp::DoubleRingBuffer<float, 16> rOutBuffer;
	float recIndex = 0.f;

	void process(const ProcessArgs& args) override {
		// Switches
		bool jumpToRecordingStart = params[JUMP_MODE_SWITCH_PARAM].getValue() > 0;
		bool jumpBack = params[JUMP_DIR_SWITCH_PARAM].getValue() > 0;
		bool blendAvg = params[BLEND_MODE_SWITCH_PARAM].getValue() > 0;

		// Knobs w/ jack summing
		float speed = clamp(params[SPEED_KNOB_PARAM].getValue() + params[SPEED_ATTENUVERTER_PARAM].getValue() * inputs[SPEED_INPUT_INPUT].getVoltage(), -5.f, 5.f);
		float delay = clamp(params[DELAY_KNOB_PARAM].getValue() + inputs[DELAY_INPUT_INPUT].getVoltage(), 0.f, 5.f);
		float blend = clamp(params[BLEND_KNOB_PARAM].getValue() + inputs[BLEND_INPUT_INPUT].getVoltage(), -5.f, 5.f);

		// Toggle recording state based on button & gate
		bool recJackTriggered = recTrigger.process(params[REC_INPUT_INPUT].getValue(), 0.1f, 1.f);
		bool recButtonTriggered = recButtonTrigger.process(params[REC_BUTTON_PARAM].getValue() > 0.5f);
		recording ^= recJackTriggered ^ recButtonTriggered;

		// Decide whether or not to jump based on button & gate
		bool jumpJackTriggered = jumpTrigger.process(params[JUMP_INPUT_INPUT].getValue(), 0.1f, 1.f);
		bool jumpButtonTriggered = jumpButtonTrigger.process(params[JUMP_BUTTON_PARAM].getValue() > 0.5f);
		bool jumping = jumpJackTriggered || jumpButtonTriggered;

		// Scale abs(speed) exponentially, where -2.5V/2.5V is 1x speed
		bool reverse = speed < 0.f;
		speed = dsp::exp2_taylor5(std::fabs(speed) - 2.5f);

		// Scale delay from -5V to 5V (linear) to 0s to 2s (exponential)
		float delaySeconds = /* if */ (delay >= 2.5f) ?
			// Exponential from .5s to 2s from noon to 5:00 knob positions
			dsp::exp2_taylor5(delay * (6.f / 5.f) - 4.f)
		: /* else if */ ((delay >= 1.f) ?
			// Exponential from .125s to .5s from 9:00 to noon knob positions
			dsp::exp2_taylor5(delay * (4.f / 3.f) - (13.f / 3.f))
		: /* else */
			// Linear from 0s to .125s from 7:00 to 9:00 knob positions
			delay * .125f
		);
		float delaySamples = delaySeconds * args.sampleRate;

		// Test clock blink
		testClockPhase += speed * args.sampleTime;
		if (testClockPhase >= 1.f) {
			testClockPhase -= 1.f;
		}

		if (lightDivider.process()) {
			float lightTime = args.sampleTime * lightDivider.getDivision();
			lights[REC_LED_LIGHT].setBrightnessSmooth(recording ? 1.f : 0.f, lightTime);
			if (testClockPhase >= 0.5f) {
				lights[JUMP_TOLERANCE_LED_LIGHT].setBrightness(1.f);
			} else {
				lights[JUMP_TOLERANCE_LED_LIGHT].setBrightnessSmooth(0.f, lightTime);
			}
		}
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

		addParam(createParamCentered<VCVButton>(mm2px(Vec(4.471, 16.529)), module, Vareo::REC_BUTTON_PARAM));
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

/***************************************************************************
 *  gdpd.h
 *  Part of GdPd
 *  2023- see AUTHORS
 *  https://gitlab.univ-lille.fr/ivmi
 ****************************************************************************/
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 130, Boston, MA 02111-1307, USA.
 */



#ifndef GDPD_CLASS_H
#define GDPD_CLASS_H

#ifdef WIN32
#include <windows.h>
#endif

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/audio_effect.hpp>
#include <godot_cpp/classes/audio_effect_instance.hpp>
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_microphone.hpp>
/*
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_playback_resampled.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
*/
#include <godot_cpp/classes/audio_frame.hpp>

#include "PdBase.hpp"
#include "RtAudio.h"

#include <lo/lo.h>

using namespace godot;

//class GdPdStreamPlayback;
class GdPdEffect;


class GdPd : public Node, public pd::PdReceiver {
	GDCLASS(GdPd, Node);

protected:
	static void _bind_methods();
    static int oscHandler(const char *path, const char *types, 
                            lo_arg ** argv,
                            int argc, lo_message msg, void *user_data) {
        GdPd* gdpd = (GdPd*)user_data; 
        gdpd->handleOSC(path, types, argv, argc, msg);
        return 0;
    }
    static void errorHandler(int num, const char *msg, const char *path){
        std::cout<<"GdPd Error : "<<msg<<" "<<path<<std::endl;
    }
    void handleOSC(const char* path, const char* types, 
                    lo_arg** argv, int argc, lo_message msg);
    void recurseCopy(String fromPath, String toPath);

private:
    bool m_realtime;
	float *m_inBuf; 
	float *m_outBuf;
	Array* m_messages;
	pd::PdBase m_pd;
	pd::Patch m_patch;
	RtAudio m_audio;
	unsigned int m_bufferFrames;
	float m_vol;
	int m_nbInputs;
	int m_nbOutputs;
	int m_sampleRate;
	int m_inputDevice;
	int m_outputDevice;
	std::map<std::string, pd::Patch> m_patchsMap;
    bool m_rtDefaults;

    AudioStreamPlayer* m_player;
    AudioStreamMicrophone* m_mic;
    GdPdEffect* m_effect;

	bool m_init;

    enum Mode {OSC, AUDIO, AUDIO_RT};
    int mode;

    String pd_patch;
    String pd_folder;
    bool m_patchOpened;
    String addon_patches_folder;
    std::string m_pdPatchStr;
    std::string m_pdDirStr;

    String application_name;
    std::string m_applicationNameStr;

    const int SENDER_PORT = 9211;
    const int RECEIVER_PORT = 9212;
    const std::string MULTICAST_ADDRESS = "239.210.211.212";
    String osc_address;
    lo_server m_receiver;
    lo_address m_destination;
    int m_senderPort;
    int m_receiverPort;
    bool m_oscBundleStarted;
    bool m_oscBundleFilled;
    lo_bundle m_oscBundle;

public:

	GdPd();
	virtual ~GdPd();

    inline void set_mode(const int p_mode){mode=p_mode;}
    inline int get_mode() const {return mode;}

	Array get_available_input_devices();
	Array get_available_output_devices();
	void set_rt_devices(String inputDevice, String outputDevice);
	void set_rt_parameters(int nbInputs, int nbOutputs, int sampleRate, int bufferSize);
    void set_rt_defaults();

	int start();
	void stop();

    void _process(double delta) override;

	bool open_patch();
	void close_patch();

    inline void set_pd_folder(const String p_folder){pd_folder=p_folder;}
    inline String get_pd_folder() const{return pd_folder;}
    inline void set_pd_patch(const String p_patch){pd_patch=p_patch;}
    inline String get_pd_patch() const{return pd_patch;}
    inline void set_addon_patches_folder(const String p_addon_patches_folder){
        addon_patches_folder=p_addon_patches_folder;
    }
    inline String get_addon_patches_folder() const{return addon_patches_folder;}

    inline void set_application_name(const String p_application_name){
        application_name=p_application_name;
        m_applicationNameStr = application_name.utf8().get_data();
    }
    inline String get_application_name() const{return application_name;}

    inline void set_osc_address(const String p_osc_address) { 
        osc_address = p_osc_address;
    }
    inline String get_osc_address() const{return osc_address;}

    void send(String destination, Array arguments);

    void begin_bundle();
    void add_to_bundle(String address, Array arguments);
    void send_bundle();

    void _send(String destination, Array arguments, bool add_to_bundle);

	virtual void print(const std::string& message) override;

	//libpd hooks
	void receiveList(const std::string& dest, const pd::List& list) override;

	//rtaudio
	static int audioCallback(void *outputBuffer, void *inputBuffer, 
							 unsigned int nBufferFrames, double streamTime, 
							 RtAudioStreamStatus status, void *userData);
	void processAudio(void *outputBuffer, void *inputBuffer, 
					   unsigned int nBufferFrames);
};


class GdPdEffectInstance;;

class GdPdEffectInstance : public AudioEffectInstance {

    GDCLASS(GdPdEffectInstance, AudioEffectInstance);
    friend class GdPdEffect;
    Ref<GdPdEffect> m_base;
    std::vector<float> m_inputBuffer;
    std::vector<float> m_outputBuffer;

protected:
    static void _bind_methods();

public:
    void _process(const void *inputBuffer, 
                  AudioFrame *outputBuffer, 
                  int32_t p_frame_count) override;
};

class GdPdEffect : public AudioEffect {
    GDCLASS(GdPdEffect, AudioEffect);
    friend class GdPdEffectInstance;

protected:
    static void _bind_methods();
    GdPd* m_gdpd;

public:
    Ref<AudioEffectInstance> _instantiate() override;

    GdPdEffect();
    ~GdPdEffect();

    void setGdPd(GdPd* gdpd){m_gdpd=gdpd;}
    GdPd* getGdPd(){return m_gdpd;}
};

/*
class GdPdStream : public AudioStream {
	GDCLASS(GdPdStream, AudioStream);
    friend class GdPdStreamPlayback;

private:
    GdPd* m_gdpd;

protected:
	static void _bind_methods(){}

public:
	GdPdStream();
    Ref<AudioStreamPlayback> _instantiate_playback() const override;

    virtual String _get_stream_name() const override {return "GdPd";}
    virtual double _get_length() const override { return 0; }

    void setGdPd(GdPd* gdpd){m_gdpd=gdpd;}
    GdPd* getGdPd(){return m_gdpd;}
};


class GdPdStreamPlayback: public AudioStreamPlaybackResampled {
	GDCLASS(GdPdStreamPlayback, AudioStreamPlaybackResampled);
    friend class GdPdStream;

protected:
    Ref<GdPdStream> m_base;
    std::vector<float> m_buffer;
    bool active;
    void *pcm_buffer;

	static void _bind_methods(){}

public:
    GdPdStreamPlayback();
    ~GdPdStreamPlayback();

    int32_t _mix_resampled(AudioFrame *dst_buffer, int32_t frame_count) override;
    float _get_stream_sampling_rate() const override;

    bool _is_playing() const override {return active;}
    void _start(double from_pos) override {active=true;}
    void _seek(double position) override {}
    void _stop() override {active=false;}
};
*/

#endif

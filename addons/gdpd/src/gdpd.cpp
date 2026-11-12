/***************************************************************************
 *  gdpd.cpp
 *  Part of GdPd
 *  2023- see README for AUTHORS
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

#include "gdpd.h"


#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/audio_server.hpp>

using namespace godot;

const int PCM_BUFFER_SIZE = 4096 * 4;

void GdPd::_bind_methods() {
	ADD_GROUP("GdPd", "gdpd_");

	ClassDB::bind_method(D_METHOD("get_available_input_devices"),
					&GdPd::get_available_input_devices);
    ClassDB::bind_method(D_METHOD("get_available_output_devices"),
					&GdPd::get_available_output_devices);
    ClassDB::bind_method(D_METHOD("set_rt_devices"), &GdPd::set_rt_devices);
    ClassDB::bind_method(D_METHOD("set_rt_parameters"), &GdPd::set_rt_parameters);
    ClassDB::bind_method(D_METHOD("set_rt_defaults"), &GdPd::set_rt_defaults);
    ClassDB::bind_method(D_METHOD("start"), &GdPd::start);
    ClassDB::bind_method(D_METHOD("stop"), &GdPd::stop);
    ClassDB::bind_method(D_METHOD("close_patch"), &GdPd::close_patch);

    ClassDB::bind_method(D_METHOD("set_mode", "p_mode"), &GdPd::set_mode);
    ClassDB::bind_method(D_METHOD("get_mode"), &GdPd::get_mode);

    ClassDB::bind_method(D_METHOD("set_osc_address", "p_osc_address"), 
                         &GdPd::set_osc_address);
    ClassDB::bind_method(D_METHOD("get_osc_address"), 
                         &GdPd::get_osc_address);

    ClassDB::bind_method(D_METHOD("set_pd_patch", "p_pd_patch"), 
                         &GdPd::set_pd_patch);
    ClassDB::bind_method(D_METHOD("get_pd_patch"), 
                         &GdPd::get_pd_patch);

    ClassDB::bind_method(D_METHOD("set_addon_patches_folder", 
                         "p_addon_patches_folder"), 
                         &GdPd::set_addon_patches_folder);
    ClassDB::bind_method(D_METHOD("get_addon_patches_folder"), 
                         &GdPd::get_addon_patches_folder);

    ClassDB::bind_method(D_METHOD("set_application_name", 
                         "p_application_name"), 
                         &GdPd::set_application_name);
    ClassDB::bind_method(D_METHOD("get_application_name"), 
                         &GdPd::get_application_name);

    ClassDB::bind_method(D_METHOD("send"), &GdPd::send);
    ClassDB::bind_method(D_METHOD("begin_bundle"), &GdPd::begin_bundle);
    ClassDB::bind_method(D_METHOD("add_to_bundle"), &GdPd::add_to_bundle);
    ClassDB::bind_method(D_METHOD("send_bundle"), &GdPd::send_bundle);


    ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", 
                    PROPERTY_HINT_ENUM, "OSC, AUDIO, AUDIO_RT", 
                    PROPERTY_USAGE_DEFAULT), 
                 "set_mode", "get_mode");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "osc_address"), 
                 "set_osc_address", "get_osc_address");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "pd_patch", PROPERTY_HINT_FILE), 
                 "set_pd_patch", "get_pd_patch");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "addon_patches_folder"), 
                 "set_addon_patches_folder", "get_addon_patches_folder");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "application_name"), 
                 "set_application_name", "get_application_name");

	ADD_SIGNAL(MethodInfo("got_message", 
                PropertyInfo(Variant::STRING, "address"), 
                PropertyInfo(Variant::ARRAY, "arguments")));
}

GdPd::GdPd() {
    mode = 0;
    m_receiver = NULL;
	m_init=false;
    m_rtDefaults=true;
    m_player=NULL;
    osc_address = String(MULTICAST_ADDRESS.c_str());
    addon_patches_folder = "res://addons/gdpd/patches";
    m_applicationNameStr = "patch";
    m_oscBundleStarted=false;
    m_oscBundleFilled=false;
}

GdPd::~GdPd() {}


int GdPd::audioCallback(void *outputBuffer, void *inputBuffer, 
					    unsigned int nBufferFrames, double streamTime, 
						RtAudioStreamStatus status, void *userData) {
	GdPd* gdpd = static_cast<GdPd*>(userData);
	gdpd->processAudio(outputBuffer, inputBuffer, nBufferFrames);
	return 0;
}


Array GdPd::get_available_input_devices() {
	Array gdlist;
    /*
	for(int d=0; d<m_audio.getDeviceCount(); d++) {
		if(m_audio.getDeviceInfo(d).inputChannels>0) {
			gdlist.push_back(m_audio.getDeviceInfo(d).name.c_str());
		}
	}*/
	return gdlist;
}

Array GdPd::get_available_output_devices() {
	Array gdlist;
    /*
	for(int d=0; d<m_audio.getDeviceCount(); d++) {
		if(m_audio.getDeviceInfo(d).outputChannels>0) {
			gdlist.push_back(m_audio.getDeviceInfo(d).name.c_str());
		}
	}*/
	return gdlist;
}


void GdPd::set_rt_devices(String inputDevice, String outputDevice) {
    /*
	std::string inpStr(inputDevice.utf8().get_data());
	std::string outStr(outputDevice.utf8().get_data());

	for(int d=0; d<m_audio.getDeviceCount(); d++) {
		std::string n = m_audio.getDeviceInfo(d).name;
		if(n.compare(inpStr)==0) {
			m_inputDevice = d;
		}
		if(n.compare(outStr)==0) {
			m_outputDevice = d;
		}
	}

	RtAudio::DeviceInfo inpInfo = m_audio.getDeviceInfo(m_inputDevice);
	RtAudio::DeviceInfo outInfo = m_audio.getDeviceInfo(m_outputDevice);
	m_nbInputs = inpInfo.inputChannels;
	m_nbOutputs = outInfo.outputChannels;
	m_sampleRate = outInfo.preferredSampleRate;
	m_bufferFrames = 128;
    */
}

void GdPd::set_rt_parameters(int nbInputs, int nbOutputs, 
                             int sampleRate, int bufferSize) {
    /*
	m_inputDevice = m_audio.getDefaultInputDevice();
	m_outputDevice = m_audio.getDefaultOutputDevice();
	RtAudio::DeviceInfo inpInfo = m_audio.getDeviceInfo(m_inputDevice);
	RtAudio::DeviceInfo outInfo = m_audio.getDeviceInfo(m_outputDevice);
	m_nbInputs 	= std::min<int>(nbInputs, inpInfo.inputChannels);
	m_nbOutputs = std::min<int>(nbOutputs, outInfo.outputChannels);
	m_sampleRate = sampleRate;
	m_bufferFrames = std::max<int>(64, bufferSize);
    */
}

void GdPd::set_rt_defaults() {

}

int GdPd::start() {
    print("Starting with mode "+std::to_string(mode));

    if(mode==OSC) {//OSC MODE
        //Create OSC sender and receiver (multicast if no address has been set)
        /*
        m_sender = lo_server_new_multicast(
                            MULTICAST_ADDRESS.c_str(), 
                            std::to_string(SENDER_PORT).c_str(), 
                            errorHandler);
                            */
        //FIXME handle given ip address
        m_destination = lo_address_new(MULTICAST_ADDRESS.c_str(),
                                        std::to_string(SENDER_PORT).c_str());
        //FIXME add local port in addition to multicast
        m_receiver = lo_server_new_multicast(
                            MULTICAST_ADDRESS.c_str(), 
                            std::to_string(RECEIVER_PORT).c_str(),NULL);
                            //errorHandler);
        /*
        m_receiver = lo_server_new_with_proto(
                            std::to_string(RECEIVER_PORT).c_str(),
                            LO_UDP, NULL);
                            */
        if(m_receiver!=NULL) {
            lo_server_add_method(m_receiver, NULL, NULL, oscHandler, this);
            m_init = true;
        }
        else {
            print("Could not create UDP connection");
        }
    }
    else { //AUDIO MODE
        
        // Retrieve defaults audio values if needed
        if(m_rtDefaults) {
            m_inputDevice = m_audio.getDefaultInputDevice();
            m_outputDevice = m_audio.getDefaultOutputDevice();
            RtAudio::DeviceInfo inpInfo = m_audio.getDeviceInfo(m_inputDevice);
            RtAudio::DeviceInfo outInfo = m_audio.getDeviceInfo(m_outputDevice);
            m_nbInputs 	= inpInfo.inputChannels;
            m_nbOutputs = outInfo.outputChannels;
            m_sampleRate = inpInfo.preferredSampleRate;
            m_bufferFrames = 128;
        }

        if(mode==AUDIO) {
            // Stereo in/out only in Godot
            m_nbInputs=2; 
            m_nbOutputs=2;
            m_sampleRate=44100;
        }

        //libpd_set_verbose(1); //FIXME Add verbose control
        if(!m_pd.init(m_nbInputs, m_nbOutputs, m_sampleRate, true)) {
            print("GDPD : Error starting libpd");
            return 1;
        }


        //start dsp
        m_pd.computeAudio(true);

        if(mode==AUDIO_RT) {
            print("Starting Audio RT");
            //intialize rtaudio
            if(m_audio.getDeviceCount()==0){
                UtilityFunctions::print("There are no available sound devices.");
                return 1;
            }

            RtAudio::StreamParameters outParams, inpParams;
            inpParams.deviceId = m_inputDevice;
            inpParams.nChannels = m_nbInputs;
            outParams.deviceId = m_outputDevice;
            outParams.nChannels = m_nbOutputs;
            print("Output channels = "+std::to_string(outParams.nChannels));
            print("Input channels = "+std::to_string(inpParams.nChannels));

            RtAudio::StreamOptions options;
            options.streamName = "GdPd";
            options.flags = RTAUDIO_SCHEDULE_REALTIME;
            if(m_audio.getCurrentApi() != RtAudio::MACOSX_CORE) {
                options.flags |= RTAUDIO_MINIMIZE_LATENCY;
            }
            try {
                m_audio.openStream(&outParams, &inpParams, RTAUDIO_FLOAT32, 
                                   m_sampleRate, &m_bufferFrames, &audioCallback, 
                                   this, &options);
                m_audio.startStream();
                print("Stream started");
            }
            catch(RtAudioError& e) {
                UtilityFunctions::print(e.getMessage().c_str());
            }
        }
        else {
            print("Starting Audio non-RT");
            // Create audio if not already there
            if(!m_player) {
                m_player = memnew(AudioStreamPlayer());
                m_mic = memnew(AudioStreamMicrophone());
                m_player->set_stream(m_mic);
                m_effect = memnew(GdPdEffect());
                m_effect->setGdPd(this);
                AudioServer::get_singleton()->add_bus_effect(0, m_effect);
                add_child(m_player);
            }
        }

        //create message hook
        m_pd.subscribe("to_godot");
        m_pd.setReceiver(this);

        // Open pd patch
        if(!pd_patch.is_empty()) {
            open_patch();
        }
        else {
            print("Please select a pd patch");
            return 1;
        }

        m_init=true;
    }

	return 0;
}

void GdPd::stop() {
	m_init=false;
    if(mode==OSC) {
        lo_server_del_method(m_receiver, NULL, NULL);
        lo_server_free(m_receiver);
        lo_address_free(m_destination);
    }
    else {
        if(mode==AUDIO_RT) {
            m_audio.stopStream();
            m_audio.closeStream();
        }
        else {
            if(m_player) {
                m_player->stop();
            }
        }
        if(m_patchOpened) {
            close_patch();
        }
        m_pd.computeAudio(false);
        m_pd.unsubscribe("to_godot");
        m_pd.clear();
    }
	print("Stopped");
}

void GdPd::begin_bundle() {
    if(mode==OSC) {
        m_oscBundleStarted=true;
        m_oscBundleFilled=false;
        m_oscBundle = lo_bundle_new(LO_TT_IMMEDIATE);
    }
}

void GdPd::send_bundle() {
    if(mode==OSC && m_oscBundleStarted && m_oscBundleFilled) {
        lo_send_bundle(m_destination, m_oscBundle);
        lo_bundle_free_recursive(m_oscBundle);
        m_oscBundleStarted=false;
    }
}

void GdPd::send(String address, Array arguments) {
    _send(address, arguments, false);
}

void GdPd::add_to_bundle(String address, Array arguments) {
    _send(address, arguments, true);
}


void GdPd::_send(String address, Array arguments, bool add_to_bundle) {
    if(!m_init) {
        return;
    }

    if(mode==OSC) {
        lo_message msg = lo_message_new();
        for(int i=0; i<arguments.size(); ++i) {
            switch(arguments[i].get_type()) {
                case Variant::INT :
                    lo_message_add_int32(msg, arguments[i]);
                    break;
                case Variant::FLOAT :
                    lo_message_add_float(msg, arguments[i]);
                    break;
                case Variant::STRING :
                    lo_message_add_string(msg, 
                            String(arguments[i]).utf8().get_data());
                    break;
                default :
                    break;
            }
        }
        
        std::string addrStr(address.utf8().get_data());
        addrStr = std::string("/gdpd/")+m_applicationNameStr+"/"+addrStr;
        if(m_oscBundleStarted && add_to_bundle) {
            m_oscBundleFilled=true;
            lo_bundle_add_message(m_oscBundle, addrStr.c_str(), msg);
        }
        else {
            lo_send_message(m_destination, addrStr.c_str(), msg);
        }
    }
    else {
        libpd_start_message(arguments.size()+2);
        libpd_add_symbol("gdpd");
        libpd_add_symbol(m_applicationNameStr.c_str());
        Array addrArray = address.split("/");
        for(int i=0; i<addrArray.size(); ++i) {
            libpd_add_symbol(String(addrArray[i]).utf8().get_data());
        }
        for(int i=0; i<arguments.size(); ++i) {
            switch(arguments[i].get_type()) {
                case Variant::INT :
                    libpd_add_float(arguments[i]);
                    break;
                case Variant::FLOAT :
                    libpd_add_float(arguments[i]);
                    break;
                case Variant::STRING : {
                    std::string symbS(String(arguments[i]).utf8().get_data());
                    libpd_add_symbol(symbS.c_str());
                    break;
               }
                default :
                    break;
            }
        }
        libpd_finish_list("from_godot");
    }
}

void GdPd::_process(double delta) {
    if(!m_init) {
        return;
    }
    if(mode==OSC) {
        if(m_receiver!=NULL) {
            int bytes = 1;
            while(bytes>0) {
                bytes = lo_server_recv_noblock(m_receiver, 0);
            }
        }
    }
    else {
        //When embedded make sure the player is playing
        if(mode==AUDIO) {
            if(m_player) {
                if(!m_player->is_playing()) {
                    m_player->play();
                }
            }
        }
        m_pd.receiveMessages();
    }
}

void GdPd::handleOSC(const char* path, const char* types, 
                     lo_arg** argv, int argc, lo_message msg) {
    std::string pathStr(path);
    if(pathStr.find("/gdpd/"+m_applicationNameStr)==0) {
        Array gdList;
        for(int i = 0; i < argc; ++i) {
            switch(types[i]) {
                case 'f' :
                    gdList.push_back(argv[i]->f);
                    break;
                case 'i' :
                    gdList.push_back(argv[i]->i);
                    break;
                case 's' :
                    gdList.push_back(String(&(argv[i]->s)));
                    break;
            }
        }
        pathStr = pathStr.substr(
                    std::string("/gdpd/"+m_applicationNameStr).length());
        emit_signal("got_message", String(pathStr.c_str()), gdList);
    }
}

void GdPd::receiveList(const std::string& dest, const pd::List& list) {
    if(dest == "to_godot") {
        //retrieve address
        int nb_addr = list.getFloat(2);
        int start_addr = 3;
        std::string addr = "/"+std::string(list.getSymbol(0).c_str())
                          +"/"+std::string(list.getSymbol(1).c_str());
        if(addr.find("/gdpd/"+m_applicationNameStr)==0) {
            addr="";
            for(int i = start_addr; i < start_addr + nb_addr; ++i) {
                addr += "/"+std::string(list.getSymbol(i).c_str());
            }

            //retrieve agruments
            Array gdList;
            for(int i = start_addr+nb_addr; i < list.len(); ++i) {
                if(list.isFloat(i)) {
                    gdList.push_back(list.getFloat(i));
                }
                else if(list.isSymbol(i)) {
                    String symbStr(list.getSymbol(i).c_str());
                    gdList.push_back(symbStr);
                }
            }
            emit_signal("got_message", String(addr.c_str()), gdList);
        }
    }
}

void GdPd::processAudio(void *outputBuffer, void *inputBuffer, 
						unsigned int nBufferFrames) {
	int ticks = nBufferFrames / libpd_blocksize();

	m_pd.processFloat(ticks, (float*)inputBuffer, (float*)outputBuffer);
}

bool GdPd::open_patch() {

    //If opening from editor, just set the paths
    if(OS::get_singleton()->has_feature("editor")) {
		String fullpath = ProjectSettings::get_singleton()
                            ->globalize_path(pd_patch);

		String pat = fullpath.get_slice("/",fullpath.get_slice_count("/")-1);
		String dir = fullpath.trim_suffix(pat);

        m_pdPatchStr = std::string(pat.utf8().get_data());
        m_pdDirStr = std::string(dir.utf8().get_data());

        print("Loading patch "+m_pdPatchStr
                +" from editor at path "
                +m_pdDirStr);
    }
    else {
        // Otherwise, copy the files to user folder
        print("Loading patch from package");

        String userPath; 

		if(OS::get_singleton()->get_name() == "Android") {
            print("On Android, so start copying files to user folder");
            String packageName = 
                (ProjectSettings::get_singleton()->globalize_path("user://"))
                    .split("/")[3];

            userPath = String("/sdcard/Android/data/")
                        + packageName
                        + String("/files/");

            print(std::string("Copying to ")+userPath.utf8().get_data());
        }
        else {
			userPath = OS::get_singleton()->get_user_data_dir()+"/";
        }


         //Get the folder containing the main pd patch
		String patch = pd_patch.get_slice("/",pd_patch.get_slice_count("/")-1);
		String patchDir = pd_patch.trim_suffix(patch).trim_prefix("res://");

        //Recursively copy this folder from res to user directory
        recurseCopy("res://"+patchDir, userPath+patchDir);

        //Also copy patches from addon patches folder
        recurseCopy(addon_patches_folder, userPath+patchDir);

        //Define new paths
        m_pdPatchStr = std::string(patch.utf8().get_data());
        m_pdDirStr = std::string((userPath+patchDir).utf8().get_data());
    }

    // Open the patch
    m_patchOpened=false;
    //FIXME fix issue in libpd with itoa in dollarzero when opening from editor
	pd::Patch p1 = m_pd.openPatch(m_pdPatchStr, m_pdDirStr);
	if(!p1.isValid()) {
		print("Could not open patch "+m_pdPatchStr);
        return false;
	}
	else {
		print("Opened patch "+m_pdPatchStr);
        m_patchOpened=true;
        std::string patchStr(pd_patch.utf8().get_data());
        m_patchsMap[patchStr] = p1;
	}
    return true;
}

void GdPd::recurseCopy(String fromPath, String toPath) {
    //Create the toPath folder
    DirAccess::make_dir_absolute(toPath);

    //print("Copying folder "+ std::string(fromPath.utf8().get_data())
    //      +" to "+std::string(toPath.utf8().get_data()));

    //For each file inside the origin directory
    PackedStringArray files = DirAccess::get_files_at(fromPath);
    for(int i=0; i<files.size(); ++i) {
        String fileName = files[i];
        Ref<DirAccess> diracc = DirAccess::open("res://");
        diracc->copy(fromPath+"/"+fileName, toPath+"/"+fileName);
        //print("Copying "+ std::string(fileName.utf8().get_data()));
    }

    //For each dir, call recursCopy
    PackedStringArray dirs = DirAccess::get_directories_at(fromPath);
    for(int i=0; i<dirs.size(); ++i) {
        String dirName = dirs[i];
        recurseCopy(fromPath+"/"+dirName, toPath+"/"+dirName);
    }
}

void GdPd::close_patch() {
    //FIXME handle multiple patches
	if(m_patchOpened) {
		m_pd.closePatch(m_pdPatchStr);
        m_patchOpened=false;
		print("Closed patch "+m_pdPatchStr);
	}
}

void GdPd::print(const std::string& message) {
	UtilityFunctions::print((std::string("GdPd : ")+message).c_str());
}

void GdPdEffectInstance::_process(const void *inpFrames, 
                                  AudioFrame *outFrames, 
                                  int32_t nbFrames)  {
    const AudioFrame *inpF = static_cast<const AudioFrame*>(inpFrames);

    //resize if needed
    if(m_inputBuffer.size()<nbFrames*2) {
        m_inputBuffer.resize(nbFrames*2);
    }
    if(m_outputBuffer.size()<nbFrames*2) {
        m_outputBuffer.resize(nbFrames*2);
    }

    //copy input buffer
    for(int i = 0; i < nbFrames; i++) {
        m_inputBuffer[i*2] = inpF[i].left;
        m_inputBuffer[i*2+1] = inpF[i].right;
    }

    //process
    m_base->getGdPd()->processAudio(m_outputBuffer.data(), 
                                    m_inputBuffer.data(), 
                                    nbFrames);

    //copy output buffer
    for(int i = 0; i < nbFrames; i++) {
        outFrames[i].left = m_outputBuffer[i*2];
        outFrames[i].right = m_outputBuffer[i*2+1];
    }
}
void GdPdEffectInstance::_bind_methods() {
}

Ref<AudioEffectInstance> GdPdEffect::_instantiate() {
	Ref<GdPdEffectInstance> inst;
	inst.instantiate();
	inst->m_base = Ref<GdPdEffect>(this);
	return inst;
}

void GdPdEffect::_bind_methods() {}

GdPdEffect::GdPdEffect() {}

GdPdEffect::~GdPdEffect() {}



/*
// GdPdStream
GdPdStream::GdPdStream() {

}

Ref<AudioStreamPlayback> GdPdStream::_instantiate_playback() const {
    Ref<GdPdStreamPlayback> gdPlayback = memnew(GdPdStreamPlayback());
    gdPlayback->m_base = Ref<GdPdStream>(this);
    return gdPlayback;
}

#define zeromem(to, count) memset(to, 0, count)

// GdPdStreamplayback
GdPdStreamPlayback::GdPdStreamPlayback() {
    m_buffer.resize(PCM_BUFFER_SIZE,0.0);
    AudioServer::get_singleton()->lock();
	pcm_buffer = memalloc(PCM_BUFFER_SIZE);
	zeromem(pcm_buffer, PCM_BUFFER_SIZE);
	AudioServer::get_singleton()->unlock();
}

GdPdStreamPlayback::~GdPdStreamPlayback() {

}

int32_t GdPdStreamPlayback::_mix_resampled(AudioFrame *buffer, 
                                           int32_t nbFrames) {
    m_buffer.resize(PCM_BUFFER_SIZE, 0.0);
    m_base->getGdPd()->processAudio(m_buffer.data(), NULL, nbFrames);
    for(int i = 0; i < nbFrames; i++) {
        buffer[i].left = m_buffer[i*2];
        buffer[i].right = m_buffer[i*2+1];
    }
    return nbFrames;
}

float GdPdStreamPlayback::_get_stream_sampling_rate() const {
    return AudioServer::get_singleton()->get_mix_rate();
}

*/

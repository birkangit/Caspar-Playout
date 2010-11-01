#include "../stdafx.h"

#include "layer.h"

#include "../producer/frame_producer.h"

#include "../frame/frame_format.h"

namespace caspar { namespace core { namespace renderer {

struct layer::implementation
{		
	implementation() : preview_frame_(nullptr), active_(nullptr), background_(nullptr), last_frame_(nullptr) {}
	
	void load(const frame_producer_ptr& frame_producer, load_option option)
	{
		if(frame_producer == nullptr) 
			BOOST_THROW_EXCEPTION(null_argument() << arg_name_info("frame_producer"));
			
		last_frame_ = nullptr;
		background_ = frame_producer;
		if(option == load_option::preview)		
		{
			last_frame_ = frame_producer->get_frame();
			if(last_frame_ != nullptr)
				last_frame_->audio_data().clear(); // No audio
			active_ = nullptr;	
		}
		else if(option == load_option::auto_play)
			play();			
	}
	
	void play()
	{			
		if(background_ != nullptr)
		{
			background_->set_leading_producer(active_);
			active_ = background_;
			background_ = nullptr;
		}

		is_paused_ = false;
	}

	void pause()
	{
		is_paused_ = true;
	}

	void stop()
	{
		active_ = nullptr;
		last_frame_ = nullptr;
	}

	void clear()
	{
		active_ = nullptr;
		background_ = nullptr;
		last_frame_ = nullptr;
	}
	
	gpu_frame_ptr get_frame()
	{		
		if(!active_ || is_paused_)
			return last_frame_;

		try
		{
			last_frame_ = active_->get_frame();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			active_ = nullptr;
			last_frame_ = nullptr;
			CASPAR_LOG(warning) << "Removed producer from layer.";
		}

		if(last_frame_ == nullptr && active_ != nullptr)
		{
			active_ = active_->get_following_producer();
			last_frame_ = get_frame();
		}
		return last_frame_;
	}	
		
	tbb::atomic<bool> is_paused_;
	gpu_frame_ptr last_frame_;
	gpu_frame_ptr preview_frame_;
	frame_producer_ptr active_;
	frame_producer_ptr background_;
};

layer::layer() : impl_(new implementation()){}
layer::layer(layer&& other) : impl_(std::move(other.impl_)){other.impl_ = nullptr;}
layer::layer(const layer& other) : impl_(new implementation(*other.impl_)) {}
layer& layer::operator=(layer&& other)
{
	impl_ = std::move(other.impl_);	
	other.impl_ = nullptr;
	return *this;
}
layer& layer::operator=(const layer& other)
{
	layer temp(other);
	impl_.swap(temp.impl_);
	return *this;
}
void layer::load(const frame_producer_ptr& frame_producer, load_option option){return impl_->load(frame_producer, option);}	
void layer::play(){impl_->play();}
void layer::pause(){impl_->pause();}
void layer::stop(){impl_->stop();}
void layer::clear(){impl_->clear();}
gpu_frame_ptr layer::get_frame() {return impl_->get_frame();}
frame_producer_ptr layer::active() const { return impl_->active_;}
frame_producer_ptr layer::background() const { return impl_->background_;}
}}}
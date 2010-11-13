#include "..\StdAfx.h"

#include "frame_producer_device.h"

#include "layer.h"

#include "../format/video_format.h"
#include "../processor/composite_frame.h"

#include "../../common/utility/scope_exit.h"
#include "../../common/utility/memory.h"

#include <boost/thread.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/foreach.hpp>

#include <tbb/concurrent_vector.h>
#include <tbb/parallel_for.h>
#include <tbb/mutex.h>
	
namespace caspar { namespace core {
	
std::vector<frame_ptr> render_frames(std::map<int, layer>& layers)
{	
	std::vector<frame_ptr> frames(layers.size(), nullptr);
	tbb::parallel_for(tbb::blocked_range<size_t>(0, frames.size()), 
	[&](const tbb::blocked_range<size_t>& r)
	{
		auto it = layers.begin();
		std::advance(it, r.begin());
		for(size_t i = r.begin(); i != r.end(); ++i, ++it)
			frames[i] = it->second.render_frame();
	});		
	return frames;
}

struct frame_producer_device::implementation : boost::noncopyable
{	
	implementation(const frame_processor_device_ptr& frame_processor)  
		: frame_processor_(frame_processor)
	{	
		is_running_ = true;
		
		render_thread_ = boost::thread([=]{run();});
	}
			
	~implementation()
	{
		frame_processor_->stop();
		is_running_ = false;
		render_thread_.join();
	}
		
	void run()
	{		
		CASPAR_LOG(info) << L"Started frame_producer_device thread";
		win32_exception::install_handler();
		
		while(is_running_)
		{
			try
			{	
				std::vector<frame_ptr> next_frames;
				{
					tbb::mutex::scoped_lock lock(layers_mutex_);	
					next_frames = render_frames(layers_);
				}				
				auto rendered_frame = std::make_shared<composite_frame>();
				boost::range::for_each(next_frames, std::bind(&composite_frame::add, rendered_frame, std::placeholders::_1));	
				frame_processor_->send(rendered_frame);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				layers_.clear();
				CASPAR_LOG(error) << "Unexpected exception. Cleared layers in render-device";
			}
		}

		CASPAR_LOG(info) << L"Ended frame_producer_device thread";
	}

	void load(int render_layer, const frame_producer_ptr& producer, load_option::type option)
	{
		producer->initialize(frame_processor_);
		tbb::mutex::scoped_lock lock(layers_mutex_);
		layers_[render_layer].load(producer, option);
	}
			
	void pause(int render_layer)
	{		
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(render_layer);
		if(it != layers_.end())
			it->second.pause();		
	}

	void play(int render_layer)
	{		
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(render_layer);
		if(it != layers_.end())
			it->second.play();		
	}

	void stop(int render_layer)
	{		
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(render_layer);
		if(it != layers_.end())
			it->second.stop();
	}

	void clear(int render_layer)
	{
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(render_layer);
		if(it != layers_.end())
			it->second.clear();		
	}
		
	void clear()
	{
		tbb::mutex::scoped_lock lock(layers_mutex_);
		layers_.clear();
	}		

	frame_producer_ptr active(int render_layer) const
	{
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(render_layer);
		return it != layers_.end() ? it->second.active() : nullptr;
	}
	
	frame_producer_ptr background(int render_layer) const
	{
		tbb::mutex::scoped_lock lock(layers_mutex_);
		auto it = layers_.find(render_layer);
		return it != layers_.end() ? it->second.background() : nullptr;
	}
	

	std::function<void(frame_ptr)> output_;
			
	boost::thread render_thread_;
					
	mutable tbb::mutex layers_mutex_;
	std::map<int, layer> layers_;
	
	tbb::atomic<bool> is_running_;	
	
	frame_processor_device_ptr frame_processor_;
};

frame_producer_device::frame_producer_device(const frame_processor_device_ptr& frame_processor) 
	: impl_(new implementation(frame_processor)){}
void frame_producer_device::load(int render_layer, const frame_producer_ptr& producer, load_option::type option){impl_->load(render_layer, producer, option);}
void frame_producer_device::pause(int render_layer){impl_->pause(render_layer);}
void frame_producer_device::play(int render_layer){impl_->play(render_layer);}
void frame_producer_device::stop(int render_layer){impl_->stop(render_layer);}
void frame_producer_device::clear(int render_layer){impl_->clear(render_layer);}
void frame_producer_device::clear(){impl_->clear();}
frame_producer_ptr frame_producer_device::active(int render_layer) const {return impl_->active(render_layer);}
frame_producer_ptr frame_producer_device::background(int render_layer) const {return impl_->background(render_layer);}
}}
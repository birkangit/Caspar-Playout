#include "../../StdAfx.h"

#include "cg_producer.h"

#include "flash_producer.h"

#include "../../Server.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

using boost::format;
using boost::io::group;
		
namespace caspar { namespace core { namespace flash {
	
struct cg_producer::implementation : boost::noncopyable
{
public:
	implementation() : flash_producer_(flash_producer(server::template_folder()+TEXT("cg.fth.18"))){}

	void clear()
	{
		flash_producer_ = flash_producer(server::template_folder()+TEXT("cg.fth.18"));
	}

	void add(int layer, const std::wstring& filename,  bool play_on_load, const std::wstring& label, const std::wstring& data)
	{
		CASPAR_LOG(info) << "Invoking add-command";
		flash_producer_->param((boost::wformat(L"<invoke name=\"Add\" returntype=\"xml\"><arguments><number>%1%</number><string>%2%</string>%3%<string>%4%</string><string><![CDATA[%5%]]></string></arguments></invoke>") % layer % filename % (play_on_load?TEXT("<true/>"):TEXT("<false/>")) % label % data).str());
	}

	void remove(int layer)
	{
		CASPAR_LOG(info) << "Invoking remove-command";
		flash_producer_->param((boost::wformat(L"<invoke name=\"Delete\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str());
	}

	void play(int layer)
	{
		CASPAR_LOG(info) << "Invoking play-command";
		flash_producer_->param((boost::wformat(L"<invoke name=\"Play\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str());
	}

	void stop(int layer, unsigned int)
	{
		CASPAR_LOG(info) << "Invoking stop-command";
		flash_producer_->param((boost::wformat(L"<invoke name=\"Stop\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str());
	}

	void next(int layer)
	{
		CASPAR_LOG(info) << "Invoking next-command";
		flash_producer_->param((boost::wformat(L"<invoke name=\"Next\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str());
	}

	void update(int layer, const std::wstring& data)
	{
		CASPAR_LOG(info) << "Invoking update-command";
		flash_producer_->param((boost::wformat(L"<invoke name=\"SetData\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array><string><![CDATA[%2%]]></string></arguments></invoke>") % layer % data).str());
	}

	void invoke(int layer, const std::wstring& label)
	{
		CASPAR_LOG(info) << "Invoking invoke-command";
		flash_producer_->param((boost::wformat(L"<invoke name=\"Invoke\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array><string>%2%</string></arguments></invoke>") % layer % label).str());
	}

	safe_ptr<draw_frame> receive()
	{
		return flash_producer_->receive();
	}
		
	void initialize(const safe_ptr<frame_processor_device>& frame_processor)
	{
		frame_processor_ = frame_processor;
		flash_producer_->initialize(frame_processor);
	}

	std::wstring print() const
	{
		return L"cg[" + flash_producer_->print() + L"]";
	}

	safe_ptr<flash_producer> flash_producer_;
	std::shared_ptr<frame_processor_device> frame_processor_;
};
	
safe_ptr<cg_producer> get_default_cg_producer(const safe_ptr<channel>& channel, int render_layer)
{	
	try
	{
		return dynamic_pointer_cast<cg_producer>(channel->foreground(render_layer).get());
	}
	catch(std::bad_cast&)
	{
		auto producer = make_safe<cg_producer>();		
		channel->load(render_layer, producer, true); 
		return producer;
	}
}

safe_ptr<frame_producer> create_ct_producer(const std::vector<std::wstring>& params) 
{
	std::wstring filename = params[0] + L".ct";
	if(!boost::filesystem::exists(filename))
		return frame_producer::empty();

	auto producer = make_safe<cg_producer>();
	producer->add(0, filename, 1);

	return producer;
}

cg_producer::cg_producer() : impl_(new implementation()){}
cg_producer::cg_producer(cg_producer&& other) : impl_(std::move(other.impl_)){}
safe_ptr<draw_frame> cg_producer::receive(){return impl_->receive();}
void cg_producer::clear(){impl_->clear();}
void cg_producer::add(int layer, const std::wstring& template_name,  bool play_on_load, const std::wstring& startFromLabel, const std::wstring& data){impl_->add(layer, template_name, play_on_load, startFromLabel, data);}
void cg_producer::remove(int layer){impl_->remove(layer);}
void cg_producer::play(int layer){impl_->play(layer);}
void cg_producer::stop(int layer, unsigned int mix_out_duration){impl_->stop(layer, mix_out_duration);}
void cg_producer::next(int layer){impl_->next(layer);}
void cg_producer::update(int layer, const std::wstring& data){impl_->update(layer, data);}
void cg_producer::invoke(int layer, const std::wstring& label){impl_->invoke(layer, label);}
void cg_producer::initialize(const safe_ptr<frame_processor_device>& frame_processor){impl_->initialize(frame_processor);}
std::wstring cg_producer::print() const{return impl_->print();}
}}}
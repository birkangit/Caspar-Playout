/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
#include "../../stdafx.h"

#include "color_producer.h"

#include "../../processor/draw_frame.h"

#include <sstream>

namespace caspar { namespace core {
	
class color_producer : public frame_producer
{
public:
	color_producer(color_producer&& other) : frame_(std::move(other.frame_)), color_str_(std::move(other.color_str_)){}

	explicit color_producer(const std::wstring& color) : color_str_(color), frame_(draw_frame::empty())
	{
		if(color.length() != 9 || color[0] != '#')
			BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("color") << arg_value_info(narrow(color)) << msg_info("Invalid color code"));
	}
	
	virtual safe_ptr<draw_frame> receive() { return frame_; }

	virtual void initialize(const safe_ptr<frame_processor_device>& frame_processor)
	{
		auto frame = frame_processor->create_frame(1, 1, pixel_format::bgra);
		auto& value = *reinterpret_cast<unsigned long*>(frame->image_data().begin());
		std::wstringstream str(color_str_.substr(1));
		str >> std::hex >> value;	
		frame_ = std::move(frame);
	}
	
	virtual std::wstring print() const { return + L"color[" + color_str_ + L"]"; }

	safe_ptr<draw_frame> frame_;
	std::wstring color_str_;
};

safe_ptr<frame_producer> create_color_producer(const std::vector<std::wstring>& params)
{
	if(params.empty() || params[0].at(0) != '#')
		return frame_producer::empty();
	return make_safe<color_producer>(params[0]);
}

}}
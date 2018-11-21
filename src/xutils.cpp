/***************************************************************************
* Copyright (c) 2018, Martin Renou, Johan Mabille, Sylvain Corlay and      *
* Wolf Vollprecht                                                          *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include <string>
#include <vector>

#include "pybind11/pybind11.h"

#include "xeus/xjson.hpp"
#include "xeus/xcomm.hpp"

#include "xutils.hpp"

namespace py = pybind11;

namespace xpyt
{

    xeus::xjson pydict_to_xjson(py::dict dict)
    {
        py::module py_json = py::module::import("json");

        return xeus::xjson::parse(static_cast<std::string>(
            py::str(py_json.attr("dumps")(dict))
        ));
    }

    py::dict xjson_to_pydict(xeus::xjson json)
    {
        py::module py_json = py::module::import("json");

        return py_json.attr("loads")(json.dump());
    }

    py::list zmq_buffers_to_pylist(const std::vector<zmq::message_t>& buffers)
    {
        py::list bufferlist;
        for (const zmq::message_t& buffer: buffers)
        {
            const char* buf = buffer.data<const char>();
            bufferlist.attr("append")(py::bytes(buf));
        }
        return bufferlist;
    }

    std::vector<zmq::message_t> pylist_to_zmq_buffers(py::list bufferlist)
    {
        std::vector<zmq::message_t> buffers;
        for (py::handle bytes_data: bufferlist)
        {
            std::string s = static_cast<std::string>(bytes_data.cast<py::bytes>());
            buffers.push_back(zmq::message_t(s.c_str(), s.size()));
        }
        return buffers;
    }

}

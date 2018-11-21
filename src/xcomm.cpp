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
#include "pybind11/embed.h"

#include "xeus/xinterpreter.hpp"
#include "xeus/xcomm.hpp"

#include "xutils.hpp"
#include "xcomm.hpp"

namespace py = pybind11;

namespace xpyt
{

    xcomm::xcomm(py::args /*args*/, py::kwargs kwargs)
        : m_comm(target(kwargs), id(kwargs))
    {
        m_comm.open(metadata(kwargs), data(kwargs), buffers(kwargs));
    }

    xcomm::~xcomm()
    {
    }

    std::string xcomm::comm_id() const
    {
        return m_comm.id();
    }

    bool xcomm::kernel() const
    {
        return true;
    }

    void xcomm::close(py::args /*args*/, py::kwargs kwargs)
    {
        m_comm.close(metadata(kwargs), data(kwargs), buffers(kwargs));
    }

    void xcomm::send(py::args /*args*/, py::kwargs kwargs)
    {
        m_comm.send(metadata(kwargs), data(kwargs), buffers(kwargs));
    }

    void xcomm::on_msg(python_callback_type callback)
    {
        m_comm.on_message(cpp_callback(callback));
    }

    void xcomm::on_close(python_callback_type callback)
    {
        m_comm.on_close(cpp_callback(callback));
    }

    xeus::xtarget* xcomm::target(py::kwargs kwargs) const
    {
        std::string target_name = static_cast<std::string>(py::str(kwargs["target_name"]));
        return xeus::get_interpreter().comm_manager().target(target_name);
    }

    xeus::xguid xcomm::id(py::kwargs kwargs) const
    {
        if (py::hasattr(kwargs, "comm_id"))
        {
            return static_cast<std::string>(py::str(kwargs["comm_id"]));
        }
        else {
            return xeus::new_xguid();
        }
    }

    xeus::xjson xcomm::data(py::kwargs kwargs) const
    {
        return pydict_to_xjson(kwargs.attr("get")("data", py::dict()));
    }

    xeus::xjson xcomm::metadata(py::kwargs kwargs) const
    {
        return pydict_to_xjson(kwargs.attr("get")("metadata", py::dict()));
    }

    auto xcomm::buffers(py::kwargs kwargs) const -> zmq_buffers_type
    {
        return pylist_to_zmq_buffers(kwargs.attr("get")("buffers", py::list()));
    }

    auto xcomm::cpp_callback(python_callback_type callback) const -> cpp_callback_type
    {
        return [callback] (const xeus::xmessage& message) {
            xeus::xjson json_message;
            json_message["header"] = message.header();
            json_message["parent_header"] = message.parent_header();
            json_message["metadata"] = message.metadata();
            json_message["content"] = message.content();

            py::dict dict_message = xjson_to_pydict(json_message);
            dict_message["buffers"] = zmq_buffers_to_pylist(message.buffers());

            callback(dict_message);
        };
    }

    PYBIND11_EMBEDDED_MODULE(xeus_python_comm, m) {
        py::class_<xcomm>(m, "XPythonComm")
            .def(py::init<py::args, py::kwargs>())
            .def("close", &xcomm::close)
            .def("send", &xcomm::send)
            .def("on_msg", &xcomm::on_msg)
            .def("on_close", &xcomm::on_close)
            .def_property_readonly("comm_id", &xcomm::comm_id)
            .def_property_readonly("kernel", &xcomm::kernel);
    }
}

#include "xeus/xinteractiveshell.hpp"

using namespace pybind11::literals;
namespace py = pybind11;

namespace xpyt
{

    void XInteractiveShell::init_magics() {
        _magic_core = py::module::import("IPython.core.magic");
        _magics_module = py::module::import("IPython.core.magics");
        _extension_module = py::module::import("IPython.core.extensions");

        magics_manager = _magic_core.attr("MagicsManager")("shell"_a=this);
        extension_manager = _extension_module.attr("ExtensionManager")("shell"_a=this);

        //shell features required by extension manager
        builtin_trap = py::module::import("contextlib").attr("nullcontext")();
        ipython_dir = "";

        py::object osm_magics =  _magics_module.attr("OSMagics");
        py::object basic_magics =  _magics_module.attr("BasicMagics");
        py::object user_magics =  _magics_module.attr("UserMagics");
        py::object extension_magics =  _magics_module.attr("ExtensionMagics");
        magics_manager.attr("register")(osm_magics);
        magics_manager.attr("register")(basic_magics);
        magics_manager.attr("register")(user_magics);
        magics_manager.attr("register")(extension_magics);
        magics_manager.attr("user_magics") = user_magics("shell"_a=this);

        //select magics supported by xeus-python
        auto line_magics = magics_manager.attr("magics")["line"];
        auto cell_magics = magics_manager.attr("magics")["cell"];
        line_magics = py::dict(
           "cd"_a=line_magics["cd"],
           "env"_a=line_magics["env"],
           "set_env"_a=line_magics["set_env"],
           "pwd"_a=line_magics["pwd"],
           "magic"_a=line_magics["magic"],
           "load_ext"_a=line_magics["load_ext"]
        );
        cell_magics = py::dict(
            "writefile"_a=cell_magics["writefile"]);

        magics_manager.attr("magics") = py::dict(
           "line"_a=line_magics,
           "cell"_a=cell_magics);
    }


    XInteractiveShell::XInteractiveShell() {
        hooks = hooks_object();
        _ipy_process = py::module::import("IPython.utils.process");
        db = py::dict();
        user_ns = py::dict("_dh"_a=py::list());
        init_magics();
    }

    py::object XInteractiveShell::system(py::str cmd) {
        return _ipy_process.attr("system")(cmd);
    }

    py::object XInteractiveShell::getoutput(py::str cmd) {
        auto stream = _ipy_process.attr("getoutput")(cmd);
        return stream.attr("splitlines")();
    }

    py::object XInteractiveShell::run_line_magic(std::string name, std::string arg)
    {

        py::object magic_method = magics_manager
            .attr("magics")["line"]
            .attr("get")(name);

        if (magic_method.is_none()) {
            PyErr_SetString(PyExc_ValueError, "magics not found");
            throw py::error_already_set();
        }

        return magic_method(arg);

    }

    py::object XInteractiveShell::run_cell_magic(std::string name, std::string arg, std::string body)
    {
        py::object magic_method = magics_manager.attr("magics")["cell"].attr("get")(name);

        if (magic_method.is_none()) {
            PyErr_SetString(PyExc_ValueError, "cell magics not found");
            throw py::error_already_set();
        }

        return magic_method(arg, body);
    }

    void XInteractiveShell::register_magic_function(py::object func, std::string magic_kind, py::object magic_name)
    {
        magics_manager.attr("register_function")(
            func, "magic_kind"_a=magic_kind, "magic_name"_a=magic_name);
    }

    void XInteractiveShell::register_magics(py::args args)
    {
        magics_manager.attr("register")(*args);
    }
}

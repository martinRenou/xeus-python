#include "pybind11/pybind11.h"
#include "xdisplay.hpp"

#ifdef __GNUC__
    #pragma GCC diagnostic ignored "-Wattributes"
#endif

namespace py = pybind11;
using namespace pybind11::literals;

namespace xpyt 
{

    struct hooks_object
    {
        static inline void show_in_pager(py::str data, py::kwargs)
        {
            xdisplay(py::dict("text/plain"_a=data), {}, {}, py::dict(), py::none(), py::none(), false, true);
        }
    };

    class xinteractive_shell
    {

    private:
        py::module _ipy_process;
        py::module _magic_core;
        py::module _magics_module;
        py::module _extension_module;

    public:
        //required by cd magic and others
        py::dict db;
        py::dict user_ns;

        //required by extension_manager
        py::object builtin_trap;
        py::str ipython_dir;

        py::object magics_manager;
        py::object extension_manager;

        hooks_object hooks;

        void register_post_execute(py::args, py::kwargs) {};
        void enable_gui(py::args, py::kwargs) {};
        void observe(py::args, py::kwargs) {};
        void showtraceback(py::args, py::kwargs) {};

        void init_magics();

        xinteractive_shell();

        py::object system(py::str cmd);

        py::object getoutput(py::str cmd); 

        py::object run_line_magic(std::string name, std::string arg);

        py::object run_cell_magic(std::string name, std::string arg, std::string body);
         
        void register_magic_function(py::object func, std::string magic_kind, py::object magic_name);

        void register_magics(py::args args);

    };
};

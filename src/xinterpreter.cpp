/***************************************************************************
* Copyright (c) 2018, Martin Renou, Johan Mabille, Sylvain Corlay and      *
* Wolf Vollprecht                                                          *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include <string>
#include <utility>
#include <vector>

#include "xeus/xinterpreter.hpp"
#include "xeus/xjson.hpp"

#include "pybind11/functional.h"

#include "xpyt_config.hpp"
#include "xinterpreter.hpp"
#include "xlogger.hpp"
#include "xdisplay.hpp"

namespace py = pybind11;

namespace xpyt
{
    void interpreter::configure_impl()
    {
    }

    interpreter::interpreter(int /*argc*/, const char* const* /*argv*/)
    {
        xeus::register_interpreter(this);
        redirect_output();
        redirect_display();

        // Monkey patching "from ipykernel.comm import Comm"
        try
        {
            py::module xeus_python_comm = py::module::import("xeus_python_comm");
            py::object xpython_comm = xeus_python_comm.attr("XPythonComm");
            py::module::import("ipykernel.comm").attr("Comm") = xpython_comm;
        }
        catch (const std::exception& /*e*/) {}
        // Monkey patching "from IPython.display import display"
        try
        {
            py::module::import("IPython.display").attr("display") = m_displayhook;
        }
        catch (const std::exception& /*e*/) {}
        // Monkey patching "from IPython import get_ipython"
        try
        {
            py::module xeus_python_kernel = py::module::import("xeus_python_kernel");
            py::module::import("IPython").attr("get_ipython") = xeus_python_kernel.attr("get_kernel");
        }
        catch (const std::exception& /*e*/) {}
    }

    interpreter::~interpreter() {}

    xeus::xjson interpreter::execute_request_impl(
        int execution_counter,
        const std::string& code,
        bool silent,
        bool /*store_history*/,
        const xeus::xjson_node* /*user_expressions*/,
        bool /*allow_stdin*/)
    {
        xeus::xjson kernel_res;

        // TODO: Check for magics
        if (code.compare("?") == 0)
        {
            std::string html_content = R"(<style>
            #pager-container {
                padding: 0;
                margin: 0;
                width: 100%;
                height: 100%;
            }
            .xpyt-iframe-pager {
                padding: 0;
                margin: 0;
                width: 100%;
                height: 100%;
                border: none;
            }
            </style>
            <iframe class="xpyt-iframe-pager" src="https://docs.python.org/"></iframe>)";

            kernel_res["status"] = "ok";
            kernel_res["payload"] = xeus::xjson::array();
            kernel_res["payload"][0] = xeus::xjson::object({
                {"data", {
                    {"text/plain", "https://docs.python.org/"},
                    {"text/html", html_content}}
                },
                {"source", "page"},
                {"start", 0}
            });
            kernel_res["user_expressions"] = xeus::xjson::object();

            return kernel_res;
        }

        try
        {
            // Import AST ans builtins modules
            py::module ast = py::module::import("ast");
            py::module builtins = py::module::import("builtins");

            // Parse code to AST
            py::object code_ast = ast.attr("parse")(code, "<string>", "exec");
            py::list expressions = code_ast.attr("body");

            // If the last statement is an expression, we compile it seperately
            // in an interactive mode (This will trigger the display hook)
            py::object last_stmt = expressions[py::len(expressions) - 1];
            if (py::isinstance(last_stmt, ast.attr("Expr")))
            {
                code_ast.attr("body").attr("pop")();

                py::list interactive_nodes;
                interactive_nodes.append(last_stmt);

                py::object interactive_ast = ast.attr("Interactive")(interactive_nodes);

                py::object compiled_code = builtins.attr("compile")(code_ast, "<ast>", "exec");
                py::object compiled_interactive_code = builtins.attr("compile")(interactive_ast, "<ast>", "single");

                m_displayhook.attr("set_execution_count")(execution_counter);

                builtins.attr("exec")(compiled_code, py::globals());
                builtins.attr("exec")(compiled_interactive_code, py::globals());
            }
            else
            {
                py::object compiled_code = builtins.attr("compile")(code_ast, "<ast>", "exec");
                builtins.attr("exec")(compiled_code, py::globals());
            }

            kernel_res["status"] = "ok";
            kernel_res["payload"] = xeus::xjson::array();
            kernel_res["user_expressions"] = xeus::xjson::object();

        } catch(const std::exception& e) {

            std::string ename = "Execution error";
            std::string evalue = e.what();
            std::vector<std::string> traceback({ename + ": " + evalue});

            if (!silent)
            {
                publish_execution_error(ename, evalue, traceback);
            }

            kernel_res["status"] = "error";
            kernel_res["ename"] = ename;
            kernel_res["evalue"] = evalue;
            kernel_res["traceback"] = traceback;
        }

        return kernel_res;
    }

    xeus::xjson interpreter::complete_request_impl(
        const std::string& /*code*/,
        int /*cursor_pos*/)
    {
        return xeus::xjson::object();
    }

    xeus::xjson interpreter::inspect_request_impl(const std::string& /*code*/,
                                                  int /*cursor_pos*/,
                                                  int /*detail_level*/)
    {
        return xeus::xjson::object();
    }

    xeus::xjson interpreter::history_request_impl(const xeus::xhistory_arguments& /*args*/)
    {
        return xeus::xjson::object();
    }

    xeus::xjson interpreter::is_complete_request_impl(const std::string& /*code*/)
    {
        return xeus::xjson::object();
    }

    xeus::xjson interpreter::kernel_info_request_impl()
    {
        xeus::xjson result;
        result["implementation"] = "xeus-python";
        result["implementation_version"] = XPYT_VERSION;

        /* The jupyter-console banner for xeus-python is the following:
            __   ________ _    _  _____       _______     _________ _    _  ____  _   _
            \ \ / /  ____| |  | |/ ____|     |  __ \ \   / /__   __| |  | |/ __ \| \ | |
             \ V /| |__  | |  | | (___ ______| |__) \ \_/ /   | |  | |__| | |  | |  \| |
              > < |  __| | |  | |\___ \______|  ___/ \   /    | |  |  __  | |  | | . ` |
             / . \| |____| |__| |____) |     | |      | |     | |  | |  | | |__| | |\  |
            /_/ \_\______|\____/|_____/      |_|      |_|     |_|  |_|  |_|\____/|_| \_|

          C++ Jupyter Kernel for Python
        */

        result["banner"] = " __   ________ _    _  _____       _______     _________ _    _  ____  _   _ \n"
                           " \\ \\ / /  ____| |  | |/ ____|     |  __ \\ \\   / /__   __| |  | |/ __ \\| \\ | |\n"
                           "  \\ V /| |__  | |  | | (___ ______| |__) \\ \\_/ /   | |  | |__| | |  | |  \\| |\n"
                           "   > < |  __| | |  | |\\___ \\______|  ___/ \\   /    | |  |  __  | |  | | . ` |\n"
                           "  / . \\| |____| |__| |____) |     | |      | |     | |  | |  | | |__| | |\\  |\n"
                           " /_/ \\_\\______|\\____/|_____/      |_|      |_|     |_|  |_|  |_|\\____/|_| \\_|\n"
                           "\n"
                           "  C++ Jupyter Kernel for Python  ";

        result["language_info"]["name"] = "python";
        result["language_info"]["version"] = "TODO";
        result["language_info"]["mimetype"] = "text/x-python";
        result["language_info"]["file_extension"] = ".py";
        return result;
    }

    void interpreter::shutdown_request_impl()
    {
        py::finalize_interpreter();
    }

    void interpreter::input_reply_impl(const std::string& /*value*/)
    {
    }

    void interpreter::redirect_output()
    {
        py::module sys = py::module::import("sys");
        py::module xeus_python_logger = py::module::import("xeus_python_logger");

        sys.attr("stdout") = xeus_python_logger.attr("XPythonLogger")("stdout");
        sys.attr("stderr") = xeus_python_logger.attr("XPythonLogger")("stderr");
    }

    void interpreter::redirect_display()
    {
        py::module sys = py::module::import("sys");
        py::module xeus_python_display = py::module::import("xeus_python_display");

        m_displayhook = xeus_python_display.attr("XPythonDisplay")();

        sys.attr("displayhook") = m_displayhook;
        py::globals()["display"] = m_displayhook;
    }
}

////////////////////////////////////////////////////////////////////////////////
#include "voronoi.h"
#include <geogram/basic/command_line.h>
#include <geogram/basic/command_line_args.h>
#include <geogram/basic/logger.h>
#include <geogram/mesh/mesh.h>
#include <geogram/mesh/mesh_io.h>
#include <geogram/mesh/mesh_reorder.h>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <CLI/CLI.hpp>
////////////////////////////////////////////////////////////////////////////////

namespace {

#ifdef WIN32
int setenv(const char *name, const char *value, int overwrite)
{
    int errcode = 0;
    if (!overwrite) {
        size_t envsize = 0;
        errcode = getenv_s(&envsize, NULL, 0, name);
        if (errcode || envsize) return errcode;
    }
    return _putenv_s(name, value);
}
#endif

}  // namespace

////////////////////////////////////////////////////////////////////////////////

// Initialize geogram library + clean up a bit its defaults behavior
void init_geogram()
{
    static bool first_time = true;

    if (first_time) {
        first_time = false;
    }
    else {
        return;
    }

    // Do not install custom signal handlers
    setenv("GEO_NO_SIGNAL_HANDLER", "1", 1);

    // Init logger first so we can hide geogram output from init
    GEO::Logger::initialize();

    // Do not show geogram output
    GEO::Logger::instance()->unregister_all_clients();
    GEO::Logger::instance()->set_quiet(true);

#if 0
    // Use the following code to disable multi-threading in geogram (for debugging purposes).
    GEO::Process::enable_multithreading(false);
    GEO::Process::set_max_threads(1);
#endif

    // Initialize global instances (e.g., logger), register MeshIOHandler, etc.
    GEO::initialize(GEO::GEOGRAM_NO_HANDLER);

    // Import standard command line arguments, and custom ones
    GEO::CmdLine::import_arg_group("standard");
    GEO::CmdLine::import_arg_group("pre");
    GEO::CmdLine::import_arg_group("algo");
    GEO::CmdLine::import_arg_group("sys");
    GEO::CmdLine::set_arg("sys:assert", "throw");
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char *argv[])
{
    // Default arguments
    struct {
        std::string input = "mesh.xyz";
        std::string output = "output.off";
    } args;

    // Parse arguments
    CLI::App app{"lattice"};
    app.add_option("input,-i,--input", args.input, "Input point cloud.")
        ->required()
        ->check(CLI::ExistingFile);
    app.add_option("output,-o,--output", args.output, "Output lattice.");
    try {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    init_geogram();

    auto operation = [&](const tbb::blocked_range<uint32_t> &range) {
        for (uint32_t i = range.begin(); i < range.end(); ++i) {
            GEO::Mesh input, output;
            mesh_load(args.input, input);
            voronoi_lattice(input, output);
            // mesh_save(output, args.output);
        }
    };
    constexpr int GRAIN_SIZE = 100;
    tbb::blocked_range<uint32_t> range(0u, 2048u, GRAIN_SIZE);
    tbb::parallel_for(range, operation);


    return 0;
}

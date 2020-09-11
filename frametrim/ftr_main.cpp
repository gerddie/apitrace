/*********************************************************************
 *
 * Copyright 2020 Collabora Ltd
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *********************************************************************/


#include "ftr_state.hpp"

#include "os_time.hpp"
#include "trace_parser.hpp"
#include "retrace.hpp"
#include "retrace_swizzle.hpp"

#include "trace_callset.hpp"
#include "trace_parser.hpp"
#include "trace_writer.hpp"

#include <limits.h> // for CHAR_MAX
#include <getopt.h>
#include <unordered_set>

using std::unordered_set;
using namespace frametrim_reverse;

struct trim_options {
    /* Frames to be included in trace. */
    trace::CallSet frames;

    /* Output filename */
    std::string output;
};

static const char *synopsis = "Create a new, retracable trace containing only one frame.";

static void
usage(void)
{
    std::cout
            << "usage: frametrim [OPTIONS] TRACE_FILE...\n"
            << synopsis << "\n"
                           "\n"
                           "    -h, --help               Show detailed help for trim options and exit\n"
                           "        --frame=FRAME        Frame the trace should be reduced to.\n"
                           "    -o, --output=TRACE_FILE  Output trace file\n"
               ;
}


enum {
    FRAMES_OPT = CHAR_MAX + 1
};

const static char *
shortOptions = "aho:x";

const static struct option
        longOptions[] = {
{"help", no_argument, 0, 'h'},
{"frames", required_argument, 0, FRAMES_OPT},
{"output", required_argument, 0, 'o'},
{0, 0, 0, 0}
};

static int trim_to_frame(const char *filename,
                         const struct trim_options& options)
{

    trace::Parser p;
    unsigned frame;

    if (!p.open(filename)) {
        std::cerr << "error: failed to open " << filename << "\n";
        return 1;
    }

    auto out_filename = options.output;

    /* Prepare output file and writer for output. */
    if (options.output.empty()) {
        os::String base(filename);
        base.trimExtension();

        out_filename = std::string(base.str()) + std::string("-trim.trace");
    }

    TraceMirror mirror;


    frame = 0;
    uint64_t callid = 0;
    std::unique_ptr<trace::Call> call(p.parse_call());
    while (call) {
        /* There's no use doing any work past the last call and frame
        * requested by the user. */
        if (frame > options.frames.getLast()) {
            break;
        }

        mirror.process(*call, options.frames.contains(frame, call->flags));

        if (call->flags & trace::CALL_FLAG_END_FRAME) {
            frame++;
        }

        callid++;
        //if (!(callid & 0xff))
            std::cerr << "\rScanning frame:" << frame << " call:" << call->no;

        call.reset(p.parse_call());
    }

    std::cerr << "\nDone at " << frame << ":" <<  callid << "\n";

    trace::Writer writer;
    if (!writer.open(out_filename.c_str(), p.getVersion(), p.getProperties())) {
        std::cerr << "error: failed to create " << out_filename << "\n";
        return 2;
    }

    auto call_ids = mirror.trace();

    std::cerr << "\nGot " << call_ids.size() << " calls\n";

    p.close();
    p.open(filename);
    call.reset(p.parse_call());

    auto callid_itr = call_ids.begin();

    while (call && callid_itr != call_ids.end()) {
        while (call && call->no != *callid_itr)
            call.reset(p.parse_call());
        if (call) {
            std::cerr << "Copy call " << call->no << "\n";
            writer.writeCall(call.get());
            call.reset(p.parse_call());
            ++callid_itr;
        }
    }

    return 0;
}




int main(int argc, char **argv)
{
    struct trim_options options;
    options.frames = trace::CallSet(trace::FREQUENCY_NONE);

    int opt;
    while ((opt = getopt_long(argc, argv, shortOptions, longOptions, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        case FRAMES_OPT:
            options.frames.merge(optarg);
            break;
        case 'o':
            options.output = optarg;
            break;
        default:
            std::cerr << "error: unexpected option `" << (char)opt << "`\n";
            usage();
            return 1;
        }
    }

    if (options.frames.getFirst() != options.frames.getLast())  {
        std::cerr << "error: Must give exactly one frame\n";
        return 1;
    }

    if (optind >= argc) {
        std::cerr << "error: apitrace trim requires a trace file as an argument.\n";
        usage();
        return 1;
    }

    if (argc > optind + 1) {
        std::cerr << "error: extraneous arguments:";
        for (int i = optind + 1; i < argc; i++) {
            std::cerr << " " << argv[i];
        }
        std::cerr << "\n";
        usage();
        return 1;
    }

    return trim_to_frame(argv[optind], options);
}

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


#include "ft_state.h"

#include "os_time.hpp"
#include "trace_parser.hpp"
#include "retrace.hpp"
#include "retrace_swizzle.hpp"

#include "trace_callset.hpp"
#include "trace_parser.hpp"
#include "trace_writer.hpp"


struct trim_options {
   /* Frames to be included in trace. */
   trace::CallSet frames;

   /* Output filename */
   std::string output;
};

static int trim_frame(const char *filename, , struct trim_options *options)
{

   trace::Parser p;
   unsigned frame;

   if (!p.open(filename)) {
       std::cerr << "error: failed to open " << filename << "\n";
       return 1;
   }

   /* Prepare output file and writer for output. */
   if (options->output.empty()) {
       os::String base(filename);
       base.trimExtension();

       options->output = std::string(base.str()) + std::string("-trim.trace");
   }

   frame = 0;
   bool in_target_frame = false;
   trace::Call *call;
   while ((call = p.parse_call())) {

       /* There's no use doing any work past the last call and frame
        * requested by the user. */
       if (frame > options->frames.getLast()) {
          delete call;
          break;
       }

       /* If this call is included in the user-specified call set,
        * then require it (and all dependencies) in the trimmed
        * output. */
       if (options->frames.contains(frame, call->flags))






       if (call->flags & trace::CALL_FLAG_END_FRAME) {
          frame++;
       }

       delete call;
   }



   return 0;
}

int main(int argc, const char **args)
{

   return 0;
}


};
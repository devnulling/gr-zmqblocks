/* -*- c++ -*- */
/* 
 * Copyright 2013 Institute for Theoretical Information Technology,
 *                RWTH Aachen University
 * 
 * Authors: Johannes Schmitz <schmitz@ti.rwth-aachen.de>
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "source_reqrep_impl.h"

namespace gr {
  namespace zmqblocks {

    source_reqrep::sptr
    source_reqrep::make(size_t itemsize, char *address)
    {
      return gnuradio::get_initial_sptr
        (new source_reqrep_impl(itemsize, address));
    }

    /*
     * The private constructor
     */
    source_reqrep_impl::source_reqrep_impl(size_t itemsize, char *address)
      : gr::sync_block("source_reqrep",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, itemsize)),
        d_itemsize(itemsize)
    {
        d_context = new zmq::context_t(1);
        d_socket = new zmq::socket_t(*d_context, ZMQ_REQ);
        d_socket->connect (address);
        std::cout << "source_reqrep on " << address << std::endl;
    }


    /*
     * Our virtual destructor.
     */
    source_reqrep_impl::~source_reqrep_impl()
    {
        delete(d_socket);
        delete(d_context);
    }

    int
    source_reqrep_impl::work(int noutput_items,
                               gr_vector_const_void_star &input_items,
                               gr_vector_void_star &output_items)
    {
        char *out = (char*)output_items[0];

        zmq::pollitem_t itemsout[] = { { *d_socket, 0, ZMQ_POLLOUT, 0 } };
        zmq::poll (&itemsout[0], 1, 0);
        //  If we got a reply, process
        if (itemsout[0].revents & ZMQ_POLLOUT) {
            // Request data, FIXME non portable
            zmq::message_t request(sizeof(int));
            memcpy ((void *) request.data (), &noutput_items, sizeof(int));
            d_socket->send(request);
        }
        zmq::pollitem_t itemsin[] = { { *d_socket, 0, ZMQ_POLLIN, 0 } };
        zmq::poll (&itemsin[0], 1, 0);
        //  If we got a reply, process
        if (itemsin[0].revents & ZMQ_POLLIN) {
            // Receive data
            zmq::message_t reply;
            d_socket->recv(&reply);

            // Copy to ouput buffer and return
            memcpy(out, (void *)reply.data(), reply.size());
            return reply.size()/d_itemsize;
        }
        return 0;
    }

  } /* namespace zmqblocks */
} /* namespace gr */


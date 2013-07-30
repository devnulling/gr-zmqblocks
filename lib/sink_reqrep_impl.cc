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
#include "sink_reqrep_impl.h"

namespace gr {
  namespace zmqblocks {

    sink_reqrep::sptr
    sink_reqrep::make(size_t itemsize, char *address)
    {
      return gnuradio::get_initial_sptr
        (new sink_reqrep_impl(itemsize, address));
    }

    /*
     * The private constructor
     */
    sink_reqrep_impl::sink_reqrep_impl(size_t itemsize, char *address)
      : gr::sync_block("sink_reqrep",
              gr::io_signature::make(1, 1, itemsize),
              gr::io_signature::make(0, 0, 0)),
        d_itemsize(itemsize)
    {
        d_context = new zmq::context_t(1);
        d_socket = new zmq::socket_t(*d_context, ZMQ_REP);
        d_socket->bind (address);
        std::cout << "sink_reqrep on " << address << std::endl;
    }

    /*
     * Our virtual destructor.
     */
    sink_reqrep_impl::~sink_reqrep_impl()
    {
        delete(d_socket);
        delete(d_context);
    }

    int
    sink_reqrep_impl::work(int noutput_items,
                             gr_vector_const_void_star &input_items,
                             gr_vector_void_star &output_items)
    {
        const char *in = (const char *) input_items[0];

        zmq::pollitem_t items[] = { { *d_socket, 0, ZMQ_POLLIN, 0 } };
        zmq::poll (&items[0], 1, 1000);
        //  If we got a reply, process
        if (items[0].revents & ZMQ_POLLIN) {
            // receive data request
            zmq::message_t request;
            d_socket->recv(&request);
            int req_output_items = *(static_cast<int*>(request.data()));

            // create message copy and send
            if (noutput_items < req_output_items) {
                zmq::message_t msg(d_itemsize*noutput_items);
                memcpy((void *)msg.data(), in, d_itemsize*noutput_items);
                d_socket->send(msg);
                return noutput_items;
            } else {
                zmq::message_t msg(d_itemsize*req_output_items);
                memcpy((void *)msg.data(), in, d_itemsize*req_output_items);
                d_socket->send(msg);
                return req_output_items;
            }
        }
        return 0;
    }
  } /* namespace zmqblocks */
} /* namespace gr */


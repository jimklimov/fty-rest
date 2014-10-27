/*  =========================================================================
    common_msg - common messages

    Codec class for common_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: common_msg.xml, or
     * The code generation script that built this file: zproto_codec_c
    ************************************************************************
                                                                        
    Copyright (C) 2014 Eaton                                            
                                                                        
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or   
    (at your option) any later version.                                 
                                                                        
    This program is distributed in the hope that it will be useful,     
    but WITHOUT ANY WARRANTY; without even the implied warranty of      
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
    GNU General Public License for more details.                        
                                                                        
    You should have received a copy of the GNU General Public License   
    along with this program.  If not, see http://www.gnu.org/licenses.  
    =========================================================================
*/

/*
@header
    common_msg - common messages
@discuss
@end
*/

#include "./common_msg.h"

//  Structure of our class

struct _common_msg_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  common_msg message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    byte errtype;                       //   An error type, defined in enum somewhere
    uint16_t errorno;                   //   An error id
    char *errmsg;                       //   A user visible error string
    zhash_t *erraux;                    //   An optional additional information about occured error
    size_t erraux_bytes;                //  Size of dictionary content
    uint32_t rowid;                     //   Id of the row processed
    char *name;                         //   Name of the client
    zmsg_t *msg;                        //   Client to be inserted
    uint32_t client_id;                 //   Unique ID of the client to be updated
    uint32_t device_id;                 //   A device id
    char *info;                         //   Information about device gathered by client
    uint32_t date;                      //   Date when this information was gathered
    uint32_t cinfo_id;                  //   Unique ID of the client info to be deleted
};

//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Put a block of octets to the frame
#define PUT_OCTETS(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
}

//  Get a block of octets from the frame
#define GET_OCTETS(host,size) { \
    if (self->needle + size > self->ceiling) \
        goto malformed; \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (host); \
    self->needle++; \
}

//  Put a 2-byte number to the frame
#define PUT_NUMBER2(host) { \
    self->needle [0] = (byte) (((host) >> 8)  & 255); \
    self->needle [1] = (byte) (((host))       & 255); \
    self->needle += 2; \
}

//  Put a 4-byte number to the frame
#define PUT_NUMBER4(host) { \
    self->needle [0] = (byte) (((host) >> 24) & 255); \
    self->needle [1] = (byte) (((host) >> 16) & 255); \
    self->needle [2] = (byte) (((host) >> 8)  & 255); \
    self->needle [3] = (byte) (((host))       & 255); \
    self->needle += 4; \
}

//  Put a 8-byte number to the frame
#define PUT_NUMBER8(host) { \
    self->needle [0] = (byte) (((host) >> 56) & 255); \
    self->needle [1] = (byte) (((host) >> 48) & 255); \
    self->needle [2] = (byte) (((host) >> 40) & 255); \
    self->needle [3] = (byte) (((host) >> 32) & 255); \
    self->needle [4] = (byte) (((host) >> 24) & 255); \
    self->needle [5] = (byte) (((host) >> 16) & 255); \
    self->needle [6] = (byte) (((host) >> 8)  & 255); \
    self->needle [7] = (byte) (((host))       & 255); \
    self->needle += 8; \
}

//  Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    if (self->needle + 1 > self->ceiling) \
        goto malformed; \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) \
        goto malformed; \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) \
        goto malformed; \
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
}

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) \
        goto malformed; \
    (host) = ((uint64_t) (self->needle [0]) << 56) \
           + ((uint64_t) (self->needle [1]) << 48) \
           + ((uint64_t) (self->needle [2]) << 40) \
           + ((uint64_t) (self->needle [3]) << 32) \
           + ((uint64_t) (self->needle [4]) << 24) \
           + ((uint64_t) (self->needle [5]) << 16) \
           + ((uint64_t) (self->needle [6]) << 8) \
           +  (uint64_t) (self->needle [7]); \
    self->needle += 8; \
}

//  Put a string to the frame
#define PUT_STRING(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER1 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a string from the frame
#define GET_STRING(host) { \
    size_t string_size; \
    GET_NUMBER1 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  Put a long string to the frame
#define PUT_LONGSTR(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER4 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a long string from the frame
#define GET_LONGSTR(host) { \
    size_t string_size; \
    GET_NUMBER4 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}


//  --------------------------------------------------------------------------
//  Create a new common_msg

common_msg_t *
common_msg_new (int id)
{
    common_msg_t *self = (common_msg_t *) zmalloc (sizeof (common_msg_t));
    self->id = id;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the common_msg

void
common_msg_destroy (common_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        common_msg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        free (self->errmsg);
        zhash_destroy (&self->erraux);
        free (self->name);
        zmsg_destroy (&self->msg);
        free (self->info);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Parse a common_msg from zmsg_t. Returns a new object, or NULL if
//  the message could not be parsed, or was NULL. Destroys msg and 
//  nullifies the msg reference.

common_msg_t *
common_msg_decode (zmsg_t **msg_p)
{
    assert (msg_p);
    zmsg_t *msg = *msg_p;
    if (msg == NULL)
        return NULL;
        
    common_msg_t *self = common_msg_new (0);
    //  Read and parse command in frame
    zframe_t *frame = zmsg_pop (msg);
    if (!frame) 
        goto empty;             //  Malformed or empty

    //  Get and check protocol signature
    self->needle = zframe_data (frame);
    self->ceiling = self->needle + zframe_size (frame);
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 9))
        goto empty;             //  Invalid signature

    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case COMMON_MSG_FAIL:
            GET_NUMBER1 (self->errtype);
            GET_NUMBER2 (self->errorno);
            GET_STRING (self->errmsg);
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                self->erraux = zhash_new ();
                zhash_autofree (self->erraux);
                while (hash_size--) {
                    char *key, *value;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->erraux, key, value);
                    free (key);
                    free (value);
                }
            }
            break;

        case COMMON_MSG_DB_OK:
            GET_NUMBER4 (self->rowid);
            break;

        case COMMON_MSG_CLIENT:
            GET_STRING (self->name);
            break;

        case COMMON_MSG_INSERT_CLIENT:
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->msg = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->msg, zmsg_pop (msg));
            break;

        case COMMON_MSG_UPDATE_CLIENT:
            GET_NUMBER4 (self->client_id);
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->msg = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->msg, zmsg_pop (msg));
            break;

        case COMMON_MSG_DELETE_CLIENT:
            GET_NUMBER4 (self->client_id);
            break;

        case COMMON_MSG_RETURN_CLIENT:
            GET_NUMBER4 (self->client_id);
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->msg = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->msg, zmsg_pop (msg));
            break;

        case COMMON_MSG_CLIENT_INFO:
            GET_NUMBER4 (self->client_id);
            GET_NUMBER4 (self->device_id);
            GET_STRING (self->info);
            GET_NUMBER4 (self->date);
            break;

        case COMMON_MSG_INSERT_CINFO:
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->msg = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->msg, zmsg_pop (msg));
            break;

        case COMMON_MSG_DELETE_CINFO:
            GET_NUMBER4 (self->cinfo_id);
            break;

        case COMMON_MSG_RETURN_CINFO:
            GET_NUMBER4 (self->cinfo_id);
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->msg = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->msg, zmsg_pop (msg));
            break;

        default:
            goto malformed;
    }
    //  Successful return
    zframe_destroy (&frame);
    zmsg_destroy (msg_p);
    return self;

    //  Error returns
    malformed:
        zsys_error ("malformed message '%d'\n", self->id);
    empty:
        zframe_destroy (&frame);
        zmsg_destroy (msg_p);
        common_msg_destroy (&self);
        return (NULL);
}


//  --------------------------------------------------------------------------
//  Encode common_msg into zmsg and destroy it. Returns a newly created
//  object or NULL if error. Use when not in control of sending the message.

zmsg_t *
common_msg_encode (common_msg_t **self_p)
{
    assert (self_p);
    assert (*self_p);
    
    common_msg_t *self = *self_p;
    zmsg_t *msg = zmsg_new ();

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case COMMON_MSG_FAIL:
            //  errtype is a 1-byte integer
            frame_size += 1;
            //  errorno is a 2-byte integer
            frame_size += 2;
            //  errmsg is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->errmsg)
                frame_size += strlen (self->errmsg);
            //  erraux is an array of key=value strings
            frame_size += 4;    //  Size is 4 octets
            if (self->erraux) {
                self->erraux_bytes = 0;
                //  Add up size of dictionary contents
                char *item = (char *) zhash_first (self->erraux);
                while (item) {
                    self->erraux_bytes += 1 + strlen ((const char *) zhash_cursor (self->erraux));
                    self->erraux_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->erraux);
                }
            }
            frame_size += self->erraux_bytes;
            break;
            
        case COMMON_MSG_DB_OK:
            //  rowid is a 4-byte integer
            frame_size += 4;
            break;
            
        case COMMON_MSG_CLIENT:
            //  name is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->name)
                frame_size += strlen (self->name);
            break;
            
        case COMMON_MSG_INSERT_CLIENT:
            break;
            
        case COMMON_MSG_UPDATE_CLIENT:
            //  client_id is a 4-byte integer
            frame_size += 4;
            break;
            
        case COMMON_MSG_DELETE_CLIENT:
            //  client_id is a 4-byte integer
            frame_size += 4;
            break;
            
        case COMMON_MSG_RETURN_CLIENT:
            //  client_id is a 4-byte integer
            frame_size += 4;
            break;
            
        case COMMON_MSG_CLIENT_INFO:
            //  client_id is a 4-byte integer
            frame_size += 4;
            //  device_id is a 4-byte integer
            frame_size += 4;
            //  info is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->info)
                frame_size += strlen (self->info);
            //  date is a 4-byte integer
            frame_size += 4;
            break;
            
        case COMMON_MSG_INSERT_CINFO:
            break;
            
        case COMMON_MSG_DELETE_CINFO:
            //  cinfo_id is a 4-byte integer
            frame_size += 4;
            break;
            
        case COMMON_MSG_RETURN_CINFO:
            //  cinfo_id is a 4-byte integer
            frame_size += 4;
            break;
            
        default:
            zsys_error ("bad message type '%d', not sent\n", self->id);
            //  No recovery, this is a fatal application error
            assert (false);
    }
    //  Now serialize message into the frame
    zframe_t *frame = zframe_new (NULL, frame_size);
    self->needle = zframe_data (frame);
    PUT_NUMBER2 (0xAAA0 | 9);
    PUT_NUMBER1 (self->id);

    switch (self->id) {
        case COMMON_MSG_FAIL:
            PUT_NUMBER1 (self->errtype);
            PUT_NUMBER2 (self->errorno);
            if (self->errmsg) {
                PUT_STRING (self->errmsg);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->erraux) {
                PUT_NUMBER4 (zhash_size (self->erraux));
                char *item = (char *) zhash_first (self->erraux);
                while (item) {
                    PUT_STRING ((const char *) zhash_cursor ((zhash_t *) self->erraux));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->erraux);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty dictionary
            break;

        case COMMON_MSG_DB_OK:
            PUT_NUMBER4 (self->rowid);
            break;

        case COMMON_MSG_CLIENT:
            if (self->name) {
                PUT_STRING (self->name);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case COMMON_MSG_INSERT_CLIENT:
            break;

        case COMMON_MSG_UPDATE_CLIENT:
            PUT_NUMBER4 (self->client_id);
            break;

        case COMMON_MSG_DELETE_CLIENT:
            PUT_NUMBER4 (self->client_id);
            break;

        case COMMON_MSG_RETURN_CLIENT:
            PUT_NUMBER4 (self->client_id);
            break;

        case COMMON_MSG_CLIENT_INFO:
            PUT_NUMBER4 (self->client_id);
            PUT_NUMBER4 (self->device_id);
            if (self->info) {
                PUT_STRING (self->info);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            PUT_NUMBER4 (self->date);
            break;

        case COMMON_MSG_INSERT_CINFO:
            break;

        case COMMON_MSG_DELETE_CINFO:
            PUT_NUMBER4 (self->cinfo_id);
            break;

        case COMMON_MSG_RETURN_CINFO:
            PUT_NUMBER4 (self->cinfo_id);
            break;

    }
    //  Now send the data frame
    if (zmsg_append (msg, &frame)) {
        zmsg_destroy (&msg);
        common_msg_destroy (self_p);
        return NULL;
    }
    //  Now send the message field if there is any
    if (self->id == COMMON_MSG_INSERT_CLIENT) {
        if (self->msg) {
            zframe_t *frame = zmsg_pop (self->msg);
            while (frame) {
                zmsg_append (msg, &frame);
                frame = zmsg_pop (self->msg);
            }
        }
        else {
            zframe_t *frame = zframe_new (NULL, 0);
            zmsg_append (msg, &frame);
        }
    }
    //  Now send the message field if there is any
    if (self->id == COMMON_MSG_UPDATE_CLIENT) {
        if (self->msg) {
            zframe_t *frame = zmsg_pop (self->msg);
            while (frame) {
                zmsg_append (msg, &frame);
                frame = zmsg_pop (self->msg);
            }
        }
        else {
            zframe_t *frame = zframe_new (NULL, 0);
            zmsg_append (msg, &frame);
        }
    }
    //  Now send the message field if there is any
    if (self->id == COMMON_MSG_RETURN_CLIENT) {
        if (self->msg) {
            zframe_t *frame = zmsg_pop (self->msg);
            while (frame) {
                zmsg_append (msg, &frame);
                frame = zmsg_pop (self->msg);
            }
        }
        else {
            zframe_t *frame = zframe_new (NULL, 0);
            zmsg_append (msg, &frame);
        }
    }
    //  Now send the message field if there is any
    if (self->id == COMMON_MSG_INSERT_CINFO) {
        if (self->msg) {
            zframe_t *frame = zmsg_pop (self->msg);
            while (frame) {
                zmsg_append (msg, &frame);
                frame = zmsg_pop (self->msg);
            }
        }
        else {
            zframe_t *frame = zframe_new (NULL, 0);
            zmsg_append (msg, &frame);
        }
    }
    //  Now send the message field if there is any
    if (self->id == COMMON_MSG_RETURN_CINFO) {
        if (self->msg) {
            zframe_t *frame = zmsg_pop (self->msg);
            while (frame) {
                zmsg_append (msg, &frame);
                frame = zmsg_pop (self->msg);
            }
        }
        else {
            zframe_t *frame = zframe_new (NULL, 0);
            zmsg_append (msg, &frame);
        }
    }
    //  Destroy common_msg object
    common_msg_destroy (self_p);
    return msg;
}


//  --------------------------------------------------------------------------
//  Receive and parse a common_msg from the socket. Returns new object or
//  NULL if error. Will block if there's no message waiting.

common_msg_t *
common_msg_recv (void *input)
{
    assert (input);
    zmsg_t *msg = zmsg_recv (input);
    if (!msg)
        return NULL;            //  Interrupted
    //  If message came from a router socket, first frame is routing_id
    zframe_t *routing_id = NULL;
    if (zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER) {
        routing_id = zmsg_pop (msg);
        //  If message was not valid, forget about it
        if (!routing_id || !zmsg_next (msg))
            return NULL;        //  Malformed or empty
    }
    common_msg_t *common_msg = common_msg_decode (&msg);
    if (common_msg && zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER)
        common_msg->routing_id = routing_id;

    return common_msg;
}


//  --------------------------------------------------------------------------
//  Receive and parse a common_msg from the socket. Returns new object,
//  or NULL either if there was no input waiting, or the recv was interrupted.

common_msg_t *
common_msg_recv_nowait (void *input)
{
    assert (input);
    zmsg_t *msg = zmsg_recv_nowait (input);
    if (!msg)
        return NULL;            //  Interrupted
    //  If message came from a router socket, first frame is routing_id
    zframe_t *routing_id = NULL;
    if (zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER) {
        routing_id = zmsg_pop (msg);
        //  If message was not valid, forget about it
        if (!routing_id || !zmsg_next (msg))
            return NULL;        //  Malformed or empty
    }
    common_msg_t *common_msg = common_msg_decode (&msg);
    if (common_msg && zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER)
        common_msg->routing_id = routing_id;

    return common_msg;
}


//  --------------------------------------------------------------------------
//  Send the common_msg to the socket, and destroy it
//  Returns 0 if OK, else -1

int
common_msg_send (common_msg_t **self_p, void *output)
{
    assert (self_p);
    assert (*self_p);
    assert (output);

    //  Save routing_id if any, as encode will destroy it
    common_msg_t *self = *self_p;
    zframe_t *routing_id = self->routing_id;
    self->routing_id = NULL;

    //  Encode common_msg message to a single zmsg
    zmsg_t *msg = common_msg_encode (self_p);
    
    //  If we're sending to a ROUTER, send the routing_id first
    if (zsocket_type (zsock_resolve (output)) == ZMQ_ROUTER) {
        assert (routing_id);
        zmsg_prepend (msg, &routing_id);
    }
    else
        zframe_destroy (&routing_id);
        
    if (msg && zmsg_send (&msg, output) == 0)
        return 0;
    else
        return -1;              //  Failed to encode, or send
}


//  --------------------------------------------------------------------------
//  Send the common_msg to the output, and do not destroy it

int
common_msg_send_again (common_msg_t *self, void *output)
{
    assert (self);
    assert (output);
    self = common_msg_dup (self);
    return common_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Encode FAIL message

zmsg_t * 
common_msg_encode_fail (
    byte errtype,
    uint16_t errorno,
    const char *errmsg,
    zhash_t *erraux)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_FAIL);
    common_msg_set_errtype (self, errtype);
    common_msg_set_errorno (self, errorno);
    common_msg_set_errmsg (self, errmsg);
    zhash_t *erraux_copy = zhash_dup (erraux);
    common_msg_set_erraux (self, &erraux_copy);
    return common_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode DB_OK message

zmsg_t * 
common_msg_encode_db_ok (
    uint32_t rowid)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_DB_OK);
    common_msg_set_rowid (self, rowid);
    return common_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode CLIENT message

zmsg_t * 
common_msg_encode_client (
    const char *name)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_CLIENT);
    common_msg_set_name (self, name);
    return common_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode INSERT_CLIENT message

zmsg_t * 
common_msg_encode_insert_client (
    zmsg_t *msg)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_INSERT_CLIENT);
    zmsg_t *msg_copy = zmsg_dup (msg);
    common_msg_set_msg (self, &msg_copy);
    return common_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode UPDATE_CLIENT message

zmsg_t * 
common_msg_encode_update_client (
    uint32_t client_id,
    zmsg_t *msg)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_UPDATE_CLIENT);
    common_msg_set_client_id (self, client_id);
    zmsg_t *msg_copy = zmsg_dup (msg);
    common_msg_set_msg (self, &msg_copy);
    return common_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode DELETE_CLIENT message

zmsg_t * 
common_msg_encode_delete_client (
    uint32_t client_id)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_DELETE_CLIENT);
    common_msg_set_client_id (self, client_id);
    return common_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode RETURN_CLIENT message

zmsg_t * 
common_msg_encode_return_client (
    uint32_t client_id,
    zmsg_t *msg)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_RETURN_CLIENT);
    common_msg_set_client_id (self, client_id);
    zmsg_t *msg_copy = zmsg_dup (msg);
    common_msg_set_msg (self, &msg_copy);
    return common_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode CLIENT_INFO message

zmsg_t * 
common_msg_encode_client_info (
    uint32_t client_id,
    uint32_t device_id,
    const char *info,
    uint32_t date)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_CLIENT_INFO);
    common_msg_set_client_id (self, client_id);
    common_msg_set_device_id (self, device_id);
    common_msg_set_info (self, info);
    common_msg_set_date (self, date);
    return common_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode INSERT_CINFO message

zmsg_t * 
common_msg_encode_insert_cinfo (
    zmsg_t *msg)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_INSERT_CINFO);
    zmsg_t *msg_copy = zmsg_dup (msg);
    common_msg_set_msg (self, &msg_copy);
    return common_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode DELETE_CINFO message

zmsg_t * 
common_msg_encode_delete_cinfo (
    uint32_t cinfo_id)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_DELETE_CINFO);
    common_msg_set_cinfo_id (self, cinfo_id);
    return common_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode RETURN_CINFO message

zmsg_t * 
common_msg_encode_return_cinfo (
    uint32_t cinfo_id,
    zmsg_t *msg)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_RETURN_CINFO);
    common_msg_set_cinfo_id (self, cinfo_id);
    zmsg_t *msg_copy = zmsg_dup (msg);
    common_msg_set_msg (self, &msg_copy);
    return common_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Send the FAIL to the socket in one step

int
common_msg_send_fail (
    void *output,
    byte errtype,
    uint16_t errorno,
    const char *errmsg,
    zhash_t *erraux)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_FAIL);
    common_msg_set_errtype (self, errtype);
    common_msg_set_errorno (self, errorno);
    common_msg_set_errmsg (self, errmsg);
    zhash_t *erraux_copy = zhash_dup (erraux);
    common_msg_set_erraux (self, &erraux_copy);
    return common_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the DB_OK to the socket in one step

int
common_msg_send_db_ok (
    void *output,
    uint32_t rowid)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_DB_OK);
    common_msg_set_rowid (self, rowid);
    return common_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CLIENT to the socket in one step

int
common_msg_send_client (
    void *output,
    const char *name)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_CLIENT);
    common_msg_set_name (self, name);
    return common_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the INSERT_CLIENT to the socket in one step

int
common_msg_send_insert_client (
    void *output,
    zmsg_t *msg)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_INSERT_CLIENT);
    zmsg_t *msg_copy = zmsg_dup (msg);
    common_msg_set_msg (self, &msg_copy);
    return common_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the UPDATE_CLIENT to the socket in one step

int
common_msg_send_update_client (
    void *output,
    uint32_t client_id,
    zmsg_t *msg)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_UPDATE_CLIENT);
    common_msg_set_client_id (self, client_id);
    zmsg_t *msg_copy = zmsg_dup (msg);
    common_msg_set_msg (self, &msg_copy);
    return common_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the DELETE_CLIENT to the socket in one step

int
common_msg_send_delete_client (
    void *output,
    uint32_t client_id)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_DELETE_CLIENT);
    common_msg_set_client_id (self, client_id);
    return common_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the RETURN_CLIENT to the socket in one step

int
common_msg_send_return_client (
    void *output,
    uint32_t client_id,
    zmsg_t *msg)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_RETURN_CLIENT);
    common_msg_set_client_id (self, client_id);
    zmsg_t *msg_copy = zmsg_dup (msg);
    common_msg_set_msg (self, &msg_copy);
    return common_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CLIENT_INFO to the socket in one step

int
common_msg_send_client_info (
    void *output,
    uint32_t client_id,
    uint32_t device_id,
    const char *info,
    uint32_t date)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_CLIENT_INFO);
    common_msg_set_client_id (self, client_id);
    common_msg_set_device_id (self, device_id);
    common_msg_set_info (self, info);
    common_msg_set_date (self, date);
    return common_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the INSERT_CINFO to the socket in one step

int
common_msg_send_insert_cinfo (
    void *output,
    zmsg_t *msg)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_INSERT_CINFO);
    zmsg_t *msg_copy = zmsg_dup (msg);
    common_msg_set_msg (self, &msg_copy);
    return common_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the DELETE_CINFO to the socket in one step

int
common_msg_send_delete_cinfo (
    void *output,
    uint32_t cinfo_id)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_DELETE_CINFO);
    common_msg_set_cinfo_id (self, cinfo_id);
    return common_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the RETURN_CINFO to the socket in one step

int
common_msg_send_return_cinfo (
    void *output,
    uint32_t cinfo_id,
    zmsg_t *msg)
{
    common_msg_t *self = common_msg_new (COMMON_MSG_RETURN_CINFO);
    common_msg_set_cinfo_id (self, cinfo_id);
    zmsg_t *msg_copy = zmsg_dup (msg);
    common_msg_set_msg (self, &msg_copy);
    return common_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Duplicate the common_msg message

common_msg_t *
common_msg_dup (common_msg_t *self)
{
    if (!self)
        return NULL;
        
    common_msg_t *copy = common_msg_new (self->id);
    if (self->routing_id)
        copy->routing_id = zframe_dup (self->routing_id);
    switch (self->id) {
        case COMMON_MSG_FAIL:
            copy->errtype = self->errtype;
            copy->errorno = self->errorno;
            copy->errmsg = self->errmsg? strdup (self->errmsg): NULL;
            copy->erraux = self->erraux? zhash_dup (self->erraux): NULL;
            break;

        case COMMON_MSG_DB_OK:
            copy->rowid = self->rowid;
            break;

        case COMMON_MSG_CLIENT:
            copy->name = self->name? strdup (self->name): NULL;
            break;

        case COMMON_MSG_INSERT_CLIENT:
            copy->msg = self->msg? zmsg_dup (self->msg): NULL;
            break;

        case COMMON_MSG_UPDATE_CLIENT:
            copy->client_id = self->client_id;
            copy->msg = self->msg? zmsg_dup (self->msg): NULL;
            break;

        case COMMON_MSG_DELETE_CLIENT:
            copy->client_id = self->client_id;
            break;

        case COMMON_MSG_RETURN_CLIENT:
            copy->client_id = self->client_id;
            copy->msg = self->msg? zmsg_dup (self->msg): NULL;
            break;

        case COMMON_MSG_CLIENT_INFO:
            copy->client_id = self->client_id;
            copy->device_id = self->device_id;
            copy->info = self->info? strdup (self->info): NULL;
            copy->date = self->date;
            break;

        case COMMON_MSG_INSERT_CINFO:
            copy->msg = self->msg? zmsg_dup (self->msg): NULL;
            break;

        case COMMON_MSG_DELETE_CINFO:
            copy->cinfo_id = self->cinfo_id;
            break;

        case COMMON_MSG_RETURN_CINFO:
            copy->cinfo_id = self->cinfo_id;
            copy->msg = self->msg? zmsg_dup (self->msg): NULL;
            break;

    }
    return copy;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
common_msg_print (common_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case COMMON_MSG_FAIL:
            zsys_debug ("COMMON_MSG_FAIL:");
            zsys_debug ("    errtype=%ld", (long) self->errtype);
            zsys_debug ("    errorno=%ld", (long) self->errorno);
            if (self->errmsg)
                zsys_debug ("    errmsg='%s'", self->errmsg);
            else
                zsys_debug ("    errmsg=");
            zsys_debug ("    erraux=");
            if (self->erraux) {
                char *item = (char *) zhash_first (self->erraux);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->erraux), item);
                    item = (char *) zhash_next (self->erraux);
                }
            }
            else
                zsys_debug ("(NULL)");
            break;
            
        case COMMON_MSG_DB_OK:
            zsys_debug ("COMMON_MSG_DB_OK:");
            zsys_debug ("    rowid=%ld", (long) self->rowid);
            break;
            
        case COMMON_MSG_CLIENT:
            zsys_debug ("COMMON_MSG_CLIENT:");
            if (self->name)
                zsys_debug ("    name='%s'", self->name);
            else
                zsys_debug ("    name=");
            break;
            
        case COMMON_MSG_INSERT_CLIENT:
            zsys_debug ("COMMON_MSG_INSERT_CLIENT:");
            zsys_debug ("    msg=");
            if (self->msg)
                zmsg_print (self->msg);
            else
                zsys_debug ("(NULL)");
            break;
            
        case COMMON_MSG_UPDATE_CLIENT:
            zsys_debug ("COMMON_MSG_UPDATE_CLIENT:");
            zsys_debug ("    client_id=%ld", (long) self->client_id);
            zsys_debug ("    msg=");
            if (self->msg)
                zmsg_print (self->msg);
            else
                zsys_debug ("(NULL)");
            break;
            
        case COMMON_MSG_DELETE_CLIENT:
            zsys_debug ("COMMON_MSG_DELETE_CLIENT:");
            zsys_debug ("    client_id=%ld", (long) self->client_id);
            break;
            
        case COMMON_MSG_RETURN_CLIENT:
            zsys_debug ("COMMON_MSG_RETURN_CLIENT:");
            zsys_debug ("    client_id=%ld", (long) self->client_id);
            zsys_debug ("    msg=");
            if (self->msg)
                zmsg_print (self->msg);
            else
                zsys_debug ("(NULL)");
            break;
            
        case COMMON_MSG_CLIENT_INFO:
            zsys_debug ("COMMON_MSG_CLIENT_INFO:");
            zsys_debug ("    client_id=%ld", (long) self->client_id);
            zsys_debug ("    device_id=%ld", (long) self->device_id);
            if (self->info)
                zsys_debug ("    info='%s'", self->info);
            else
                zsys_debug ("    info=");
            zsys_debug ("    date=%ld", (long) self->date);
            break;
            
        case COMMON_MSG_INSERT_CINFO:
            zsys_debug ("COMMON_MSG_INSERT_CINFO:");
            zsys_debug ("    msg=");
            if (self->msg)
                zmsg_print (self->msg);
            else
                zsys_debug ("(NULL)");
            break;
            
        case COMMON_MSG_DELETE_CINFO:
            zsys_debug ("COMMON_MSG_DELETE_CINFO:");
            zsys_debug ("    cinfo_id=%ld", (long) self->cinfo_id);
            break;
            
        case COMMON_MSG_RETURN_CINFO:
            zsys_debug ("COMMON_MSG_RETURN_CINFO:");
            zsys_debug ("    cinfo_id=%ld", (long) self->cinfo_id);
            zsys_debug ("    msg=");
            if (self->msg)
                zmsg_print (self->msg);
            else
                zsys_debug ("(NULL)");
            break;
            
    }
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
common_msg_routing_id (common_msg_t *self)
{
    assert (self);
    return self->routing_id;
}

void
common_msg_set_routing_id (common_msg_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the common_msg id

int
common_msg_id (common_msg_t *self)
{
    assert (self);
    return self->id;
}

void
common_msg_set_id (common_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
common_msg_command (common_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case COMMON_MSG_FAIL:
            return ("FAIL");
            break;
        case COMMON_MSG_DB_OK:
            return ("DB_OK");
            break;
        case COMMON_MSG_CLIENT:
            return ("CLIENT");
            break;
        case COMMON_MSG_INSERT_CLIENT:
            return ("INSERT_CLIENT");
            break;
        case COMMON_MSG_UPDATE_CLIENT:
            return ("UPDATE_CLIENT");
            break;
        case COMMON_MSG_DELETE_CLIENT:
            return ("DELETE_CLIENT");
            break;
        case COMMON_MSG_RETURN_CLIENT:
            return ("RETURN_CLIENT");
            break;
        case COMMON_MSG_CLIENT_INFO:
            return ("CLIENT_INFO");
            break;
        case COMMON_MSG_INSERT_CINFO:
            return ("INSERT_CINFO");
            break;
        case COMMON_MSG_DELETE_CINFO:
            return ("DELETE_CINFO");
            break;
        case COMMON_MSG_RETURN_CINFO:
            return ("RETURN_CINFO");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the errtype field

byte
common_msg_errtype (common_msg_t *self)
{
    assert (self);
    return self->errtype;
}

void
common_msg_set_errtype (common_msg_t *self, byte errtype)
{
    assert (self);
    self->errtype = errtype;
}


//  --------------------------------------------------------------------------
//  Get/set the errorno field

uint16_t
common_msg_errorno (common_msg_t *self)
{
    assert (self);
    return self->errorno;
}

void
common_msg_set_errorno (common_msg_t *self, uint16_t errorno)
{
    assert (self);
    self->errorno = errorno;
}


//  --------------------------------------------------------------------------
//  Get/set the errmsg field

const char *
common_msg_errmsg (common_msg_t *self)
{
    assert (self);
    return self->errmsg;
}

void
common_msg_set_errmsg (common_msg_t *self, const char *format, ...)
{
    //  Format errmsg from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->errmsg);
    self->errmsg = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get the erraux field without transferring ownership

zhash_t *
common_msg_erraux (common_msg_t *self)
{
    assert (self);
    return self->erraux;
}

//  Get the erraux field and transfer ownership to caller

zhash_t *
common_msg_get_erraux (common_msg_t *self)
{
    zhash_t *erraux = self->erraux;
    self->erraux = NULL;
    return erraux;
}

//  Set the erraux field, transferring ownership from caller

void
common_msg_set_erraux (common_msg_t *self, zhash_t **erraux_p)
{
    assert (self);
    assert (erraux_p);
    zhash_destroy (&self->erraux);
    self->erraux = *erraux_p;
    *erraux_p = NULL;
}

//  --------------------------------------------------------------------------
//  Get/set a value in the erraux dictionary

const char *
common_msg_erraux_string (common_msg_t *self, const char *key, const char *default_value)
{
    assert (self);
    const char *value = NULL;
    if (self->erraux)
        value = (const char *) (zhash_lookup (self->erraux, key));
    if (!value)
        value = default_value;

    return value;
}

uint64_t
common_msg_erraux_number (common_msg_t *self, const char *key, uint64_t default_value)
{
    assert (self);
    uint64_t value = default_value;
    char *string = NULL;
    if (self->erraux)
        string = (char *) (zhash_lookup (self->erraux, key));
    if (string)
        value = atol (string);

    return value;
}

void
common_msg_erraux_insert (common_msg_t *self, const char *key, const char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    //  Store string in hash table
    if (!self->erraux) {
        self->erraux = zhash_new ();
        zhash_autofree (self->erraux);
    }
    zhash_update (self->erraux, key, string);
    free (string);
}

size_t
common_msg_erraux_size (common_msg_t *self)
{
    return zhash_size (self->erraux);
}


//  --------------------------------------------------------------------------
//  Get/set the rowid field

uint32_t
common_msg_rowid (common_msg_t *self)
{
    assert (self);
    return self->rowid;
}

void
common_msg_set_rowid (common_msg_t *self, uint32_t rowid)
{
    assert (self);
    self->rowid = rowid;
}


//  --------------------------------------------------------------------------
//  Get/set the name field

const char *
common_msg_name (common_msg_t *self)
{
    assert (self);
    return self->name;
}

void
common_msg_set_name (common_msg_t *self, const char *format, ...)
{
    //  Format name from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->name);
    self->name = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get the msg field without transferring ownership

zmsg_t *
common_msg_msg (common_msg_t *self)
{
    assert (self);
    return self->msg;
}

//  Get the msg field and transfer ownership to caller

zmsg_t *
common_msg_get_msg (common_msg_t *self)
{
    zmsg_t *msg = self->msg;
    self->msg = NULL;
    return msg;
}

//  Set the msg field, transferring ownership from caller

void
common_msg_set_msg (common_msg_t *self, zmsg_t **msg_p)
{
    assert (self);
    assert (msg_p);
    zmsg_destroy (&self->msg);
    self->msg = *msg_p;
    *msg_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get/set the client_id field

uint32_t
common_msg_client_id (common_msg_t *self)
{
    assert (self);
    return self->client_id;
}

void
common_msg_set_client_id (common_msg_t *self, uint32_t client_id)
{
    assert (self);
    self->client_id = client_id;
}


//  --------------------------------------------------------------------------
//  Get/set the device_id field

uint32_t
common_msg_device_id (common_msg_t *self)
{
    assert (self);
    return self->device_id;
}

void
common_msg_set_device_id (common_msg_t *self, uint32_t device_id)
{
    assert (self);
    self->device_id = device_id;
}


//  --------------------------------------------------------------------------
//  Get/set the info field

const char *
common_msg_info (common_msg_t *self)
{
    assert (self);
    return self->info;
}

void
common_msg_set_info (common_msg_t *self, const char *format, ...)
{
    //  Format info from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->info);
    self->info = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the date field

uint32_t
common_msg_date (common_msg_t *self)
{
    assert (self);
    return self->date;
}

void
common_msg_set_date (common_msg_t *self, uint32_t date)
{
    assert (self);
    self->date = date;
}


//  --------------------------------------------------------------------------
//  Get/set the cinfo_id field

uint32_t
common_msg_cinfo_id (common_msg_t *self)
{
    assert (self);
    return self->cinfo_id;
}

void
common_msg_set_cinfo_id (common_msg_t *self, uint32_t cinfo_id)
{
    assert (self);
    self->cinfo_id = cinfo_id;
}



//  --------------------------------------------------------------------------
//  Selftest

int
common_msg_test (bool verbose)
{
    printf (" * common_msg: ");

    //  @selftest
    //  Simple create/destroy test
    common_msg_t *self = common_msg_new (0);
    assert (self);
    common_msg_destroy (&self);

    //  Create pair of sockets we can send through
    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    zsock_connect (input, "inproc://selftest-common_msg");

    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    zsock_bind (output, "inproc://selftest-common_msg");

    //  Encode/send/decode and verify each message type
    int instance;
    common_msg_t *copy;
    self = common_msg_new (COMMON_MSG_FAIL);
    
    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_errtype (self, 123);
    common_msg_set_errorno (self, 123);
    common_msg_set_errmsg (self, "Life is short but Now lasts for ever");
    common_msg_erraux_insert (self, "Name", "Brutus");
    common_msg_erraux_insert (self, "Age", "%d", 43);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));
        
        assert (common_msg_errtype (self) == 123);
        assert (common_msg_errorno (self) == 123);
        assert (streq (common_msg_errmsg (self), "Life is short but Now lasts for ever"));
        assert (common_msg_erraux_size (self) == 2);
        assert (streq (common_msg_erraux_string (self, "Name", "?"), "Brutus"));
        assert (common_msg_erraux_number (self, "Age", 0) == 43);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_DB_OK);
    
    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_rowid (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));
        
        assert (common_msg_rowid (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_CLIENT);
    
    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_name (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));
        
        assert (streq (common_msg_name (self), "Life is short but Now lasts for ever"));
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_INSERT_CLIENT);
    
    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    zmsg_t *insert_client_msg = zmsg_new ();
    common_msg_set_msg (self, &insert_client_msg);
    zmsg_addstr (common_msg_msg (self), "Hello, World");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));
        
        assert (zmsg_size (common_msg_msg (self)) == 1);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_UPDATE_CLIENT);
    
    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_client_id (self, 123);
    zmsg_t *update_client_msg = zmsg_new ();
    common_msg_set_msg (self, &update_client_msg);
    zmsg_addstr (common_msg_msg (self), "Hello, World");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));
        
        assert (common_msg_client_id (self) == 123);
        assert (zmsg_size (common_msg_msg (self)) == 1);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_DELETE_CLIENT);
    
    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_client_id (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));
        
        assert (common_msg_client_id (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_RETURN_CLIENT);
    
    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_client_id (self, 123);
    zmsg_t *return_client_msg = zmsg_new ();
    common_msg_set_msg (self, &return_client_msg);
    zmsg_addstr (common_msg_msg (self), "Hello, World");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));
        
        assert (common_msg_client_id (self) == 123);
        assert (zmsg_size (common_msg_msg (self)) == 1);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_CLIENT_INFO);
    
    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_client_id (self, 123);
    common_msg_set_device_id (self, 123);
    common_msg_set_info (self, "Life is short but Now lasts for ever");
    common_msg_set_date (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));
        
        assert (common_msg_client_id (self) == 123);
        assert (common_msg_device_id (self) == 123);
        assert (streq (common_msg_info (self), "Life is short but Now lasts for ever"));
        assert (common_msg_date (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_INSERT_CINFO);
    
    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    zmsg_t *insert_cinfo_msg = zmsg_new ();
    common_msg_set_msg (self, &insert_cinfo_msg);
    zmsg_addstr (common_msg_msg (self), "Hello, World");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));
        
        assert (zmsg_size (common_msg_msg (self)) == 1);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_DELETE_CINFO);
    
    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_cinfo_id (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));
        
        assert (common_msg_cinfo_id (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_RETURN_CINFO);
    
    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_cinfo_id (self, 123);
    zmsg_t *return_cinfo_msg = zmsg_new ();
    common_msg_set_msg (self, &return_cinfo_msg);
    zmsg_addstr (common_msg_msg (self), "Hello, World");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));
        
        assert (common_msg_cinfo_id (self) == 123);
        assert (zmsg_size (common_msg_msg (self)) == 1);
        common_msg_destroy (&self);
    }

    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
    return 0;
}

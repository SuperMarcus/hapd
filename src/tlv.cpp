//
// Created by Xule Zhou on 6/4/18.
//

#include "tlv.h"

tlv8_item *tlv8_parse(uint8_t * data, unsigned int length) {
    const uint8_t * end = data + length;
    auto current_item = new tlv8_item;
    auto start_item = current_item;

    while (data < end) {
        current_item->type = static_cast<tlv8_type>(data[0]);
        current_item->length = data[1];
        current_item->value = &data[2];
        current_item->offset = 0;

        current_item->next = new tlv8_item;
        current_item->next->previous = current_item;
        current_item = current_item->next;
    }

    return start_item;
}

void tlv8_free(tlv8_item *chain) {
    auto current = chain;
    while (chain != nullptr){
        current = chain;
        chain = current->next;
        delete current;
    }
}

tlv8_item *tlv8_find(tlv8_item *chain, tlv8_type type) {
    //Start searching from the start of the chain
    while (chain->previous != nullptr){
        chain = chain->previous;
    }

    while (chain != nullptr && chain->type != type){
        chain = chain->next;
    }

    return chain;
}

unsigned int tlv8_read(tlv8_item *item, uint8_t *buffer, unsigned int length) {
    unsigned int read = 0;

    //Get to the place where it has not been read
    while(item != nullptr && item->offset == 255){
        //If the next fragment does not have the same type as the current one, abort with 0 bytes read
        if(item->next->type != item->type) { return read; }
        item = item->next;
    }

    while(item != nullptr && read < length){
        unsigned int need_read = length - read;
        unsigned int can_read = item->length - item->offset;
        unsigned int to_read = can_read < need_read ? can_read : need_read;

        memcpy(buffer, item->value, to_read);
        read += to_read;
        item->offset += to_read;

        //Jump to the next fragment
        if(item->offset >= 255){
            //Return if the next item is not of the same type
            if(item->next->type != item->type){ return read; }
            item = item->next;
        }
    }

    return read;
}

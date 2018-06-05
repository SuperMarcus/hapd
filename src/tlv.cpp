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

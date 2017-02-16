//
//  StringPool.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "StringPool.hpp"

EmojicodeInstruction StringPool::poolString(const EmojicodeString &string) {
    for (size_t i = 0; i < strings_.size(); i++) {
        if (strings_[i].compare(string) == 0) {
            return static_cast<EmojicodeInstruction>(i);
        }
    }

    strings_.push_back(string);
    return static_cast<EmojicodeInstruction>(strings_.size() - 1);
}

//
// Created by Max Vissing on 2019-06-07.
//

#include "JSON.h"

JSON::JSON(size_t bufferSize, std::function<void(uint8_t *buffer, size_t size)> onFlush) : buffer((byte *) malloc(bufferSize)), size(bufferSize), pos(0), state(JSONStateStart), nestingId(0), nesting(), onFlush(std::move(onFlush)) {}

JSON::~JSON() {
    flush();
    free(buffer);
}

void JSON::flush() {
    if (!pos) {
        return;
    }

    if (onFlush) {
        onFlush(buffer, pos);
    }
    pos = 0;
}

void JSON::write(const char *format, ...) {
    va_list arg_ptr;

    va_start(arg_ptr, format);
    int len = vsnprintf((char *) buffer + pos, size - pos, format, arg_ptr);
    va_end(arg_ptr);

    if (len + pos > size - 1) {
        flush();

        va_start(arg_ptr, format);
        len = vsnprintf((char *) buffer + pos, size - pos, format, arg_ptr);
        va_end(arg_ptr);

        if (len < size) {
            pos += len;
        }
    } else {
        pos += len;
    }
}

void JSON::startObject() {
    if (state == JSONStateError) {
        return;
    }

    switch (state) {
        case JSONStateArrayItem:
            write(",");
        case JSONStateStart:
        case JSONStateObjectKey:
        case JSONStateArray:
            write("{");

            state = JSONStateObject;
            nesting[nestingId++] = JSONNestingObject;
            break;
        default:
            state = JSONStateError;
    }
}

void JSON::endObject() {
    if (state == JSONStateError) {
        return;
    }

    switch (state) {
        case JSONStateObject:
        case JSONStateObjectValue:
            write("}");

            nestingId--;
            if (!nestingId) {
                state = JSONStateEnd;
            } else {
                switch (nesting[nestingId - 1]) {
                    case JSONNestingObject:
                        state = JSONStateObjectValue;
                        break;
                    case JSONNestingArray:
                        state = JSONStateArrayItem;
                        break;
                }
            }
            break;
        default:
            state = JSONStateError;
    }
}

void JSON::startArray() {
    if (state == JSONStateError) {
        return;
    }

    switch (state) {
        case JSONStateArrayItem:
            write(",");
        case JSONStateStart:
        case JSONStateObjectKey:
        case JSONStateArray:
            write("[");

            state = JSONStateArray;
            nesting[nestingId++] = JSONNestingArray;
            break;
        default:
            state = JSONStateError;
    }
}

void JSON::endArray() {
    if (state == JSONStateError) {
        return;
    }

    switch (state) {
        case JSONStateArray:
        case JSONStateArrayItem:
            write("]");

            nestingId--;
            if (!nestingId) {
                state = JSONStateEnd;
            } else {
                switch (nesting[nestingId - 1]) {
                    case JSONNestingObject:
                        state = JSONStateObjectValue;
                        break;
                    case JSONNestingArray:
                        state = JSONStateArrayItem;
                        break;
                }
            }
            break;
        default:
            state = JSONStateError;
    }
}

void JSON::setInt(long long value) {
    if (state == JSONStateError) {
        return;
    }

    switch (state) {
        case JSONStateStart:
            write("%ld", value);
            state = JSONStateEnd;
            break;
        case JSONStateArrayItem:
            write(",");
        case JSONStateArray:
            write("%ld", value);

            state = JSONStateArrayItem;
            break;
        case JSONStateObjectKey:
            write("%ld", value);

            state = JSONStateObjectValue;
            break;
        default:
            state = JSONStateError;
    }
}

void JSON::setFloat(float value) {
    if (state == JSONStateError) {
        return;
    }

    switch (state) {
        case JSONStateStart:
            write("%1.15g", value);
            state = JSONStateEnd;
            break;
        case JSONStateArrayItem:
            write(",");
        case JSONStateArray:
            write("%1.15g", value);

            state = JSONStateArrayItem;
            break;
        case JSONStateObjectKey:
            write("%1.15g", value);

            state = JSONStateObjectValue;
            break;
        default:
            state = JSONStateError;
    }
}

void JSON::setString(const char *value) {
    if (state == JSONStateError) {
        return;
    }

    switch (state) {
        case JSONStateStart:
            write("\"%s\"", value);
            state = JSONStateEnd;
            break;
        case JSONStateArrayItem:
            write(",");
        case JSONStateArray:
            write("\"%s\"", value);

            state = JSONStateArrayItem;
            break;
        case JSONStateObjectValue:
            write(",");
        case JSONStateObject:
            write("\"%s\"", value);
            write(":");

            state = JSONStateObjectKey;
            break;
        case JSONStateObjectKey:
            write("\"%s\"", value);

            state = JSONStateObjectValue;
            break;
        default:
            state = JSONStateError;
    }
}

void JSON::setBool(bool value) {
    if (state == JSONStateError) {
        return;
    }

    switch (state) {
        case JSONStateStart:
            write((value) ? "true" : "false");
            state = JSONStateEnd;
            break;
        case JSONStateArrayItem:
            write(",");
        case JSONStateArray:
            write((value) ? "true" : "false");

            state = JSONStateArrayItem;
            break;
        case JSONStateObjectKey:
            write((value) ? "true" : "false");

            state = JSONStateObjectValue;
            break;
        default:
            state = JSONStateError;
    }
}

void JSON::setNull() {
    if (state == JSONStateError) {
        return;
    }

    switch (state) {
        case JSONStateStart:
            write("null");
            state = JSONStateEnd;
            break;
        case JSONStateArrayItem:
            write(",");
        case JSONStateArray:
            write("null");

            state = JSONStateArrayItem;
            break;
        case JSONStateObjectKey:
            write("null");

            state = JSONStateObjectValue;
            break;
        default:
            state = JSONStateError;
    }
}

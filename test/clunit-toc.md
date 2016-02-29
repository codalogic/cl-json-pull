Tests table of contents generated on Mon Feb 29 14:57:52 2016

# test-event.cpp
| Description | Line |
|-------------|------|
| struct Event | 38 |
| Event simple / convenience is_XXX methods | 61 |
| Event::is_true and is_false | 74 |
| Event::is_int | 113 |
| Event::is() [name] | 144 |
| Event::on() [name] | 158 |
| Event::to_bool | 168 |
| Event to_float, to_int | 225 |
| Event to_string, to_wstring | 274 |

# test-messages.cpp
| Description | Line |
|-------------|------|
| Reading whole messages | 67 |
| Reading multiple messages in single stream | 300 |
| Parser: illegally formed messages | 399 |
| Parser truncated input | 454 |

# test-parse.cpp
| Description | Line |
|-------------|------|
| Basic Parser | 42 |
| Repeated reads at end of message return 'End of message' | 208 |
| Parser Reading constant values | 252 |
| Parser Reading number values | 322 |
| Parser Back-to-back numbers | 366 |
| Parser Reading string values | 456 |
| Parser Reading string Unicode escapes | 483 |
| Parser Reading string with bad Unicode escapes, check parsing terminated mid-string | 545 |
| Parser::get_string() - String end quote in middle of unicode escape code | 550 |
| Parser Reading string unexpected EOF | 564 |
| Parser Read member | 574 |
| Parser::skip() | 678 |

# test-read-utf8.cpp
| Description | Line |
|-------------|------|
| Test the test ByteTestSequence | 40 |
| ReadUTF8 - test MK_STR_WITH_ZEROS | 114 |
| ReadUTF8 - UTF-8 input | 139 |
| ReadUTF8 - UTF-8 input with BOM | 176 |
| ReadUTF8 - UTF-8 input error cases | 198 |
| ReadUTF8 - UTF-16LE input | 220 |
| ReadUTF8 - UTF-16LE input with BOM | 260 |
| ReadUTF8 - UTF-16LE input - verify boundarries in UTF-8 encoding | 274 |
| ReadUTF8 - UTF-16LE input surrogates | 299 |
| ReadUTF8 - UTF-16LE input error cases | 320 |
| ReadUTF8 - UTF-16BE input | 333 |
| ReadUTF8 - UTF-16BE input with BOM | 376 |
| ReadUTF8 - UTF-16BE input - verify boundarries in UTF-8 encoding | 388 |
| ReadUTF8 - UTF-16BE input surrogates | 413 |
| ReadUTF8 - UTF-16BE input error cases | 434 |
| ReadUTF8 - UTF-32LE input | 449 |
| ReadUTF8 - UTF-32LE input with BOM | 461 |
| ReadUTF8 - UTF-32LE input - verify boundarries in UTF-8 encoding | 475 |
| ReadUTF8 - UTF-32LE input error cases | 510 |
| ReadUTF8 - UTF-32BE input | 520 |
| ReadUTF8 - UTF-32BE input with BOM | 536 |
| ReadUTF8 - UTF-32LE input - verify boundarries in UTF-8 encoding | 552 |
| ReadUTF8 - UTF-32BE input error cases | 587 |
| ReadUTF8 - Rewind | 627 |

# test-reader.cpp
| Description | Line |
|-------------|------|
| class ReaderMemory | 38 |
| class ReaderFile | 61 |
| class ReadUTF8WithUnget | 88 |

# test-todo.cpp
| Description | Line |
|-------------|------|
| TODOs | 38 |

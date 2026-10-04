# ODBlock reconstruction

Source evidence: extracted `ODBlock.obj` from the shipped `GraphicsLib.lib`, analyzed as x86 COFF with IDA. Object SHA256: `E15DA68B317E73CB190ACC414C39EB5340A5DB84AD5802A4CFFABC998D18576D`. `ODBlock-oracle.txt` records decompilation; offsets below are IDA COFF addresses (not PE virtual addresses). The working IDA database lives on C: to conserve workspace disk space.

| Offset | Evidence |
| --- | --- |
| `0x000c`, `0x01e8` | `_map`, `_invmap`: 475 bytes each, five permutations of character indexes 0..94. All 475 source map bytes were compared directly against IDA bytes; the derived inverse was verified against every original inverse byte. |
| `0x0498`, `0x04d8`, `0x0518` | Byte encryption, decryption, string encryption. Signed bytes <=32 pass through without state changes. Other bytes subtract 33, use column `state-1`, then set `state=(inputIndex+outputIndex+state)%5+1`. |
| `0x0558` | Six literal syntax errors, preserved verbatim. |
| `0x05a8`, handler `0x06a2` | `loadScript` enables whitespace skipping; leading `!` selects algorithm code `1`, initializes state to 1, and parses encrypted data. Syntax failures set `lastError` and return null. Successful loads leave the previous error unchanged. |
| `0x07d8` | Recursive constructor. Quotes turn stream whitespace skipping off/on; unquoted whitespace disappears even inside identifiers. `#` ignores until newline; `/` ignores through the next slash, matching `/* ... */`. No escape sequences exist for quotes. The first already decoded child byte is passed into the nested constructor and must not be decoded again. |
| `0x0ed0`, `0x0f00`, `0x1434` | Default terminal state, ownership/destruction, append statement and mark parent nonterminal. Empty braces remain terminal because only adding children switches the flag. |
| `0x1000`, `0x1054`, `0x125c` | `saveFile` prefixes encrypted data with literal `!1`; encrypted serialization always quotes identifiers and omits formatting. Plain serialization starts indentation at 1: root name has no tab, root braces and child names each have one tab. FILE output follows the same layout. |
| `0x13a4`, `0x13b4`, `0x13c4` | Unset identifiers return `_noname`; `isTerminal` reads the flag without resetting iteration; setter ignores null. |
| `0x15f4`..`0x1684` | Shared iterator for all child/terminal methods. `moreTerminals` advances past nonterminal children. `reset` rewinds. |
| `0x1694` | Search includes self, then immediate children or recursive depth-first traversal. Comparisons use `strncmp(...,30)`, not a full identifier comparison. |
| `0x1724`..`0x180c` | Scalar queries return success if the named block has a terminal, regardless of numeric scan success. Null numeric outputs act as presence checks. Vector query returns success when the named block exists, scans available terminals, and leaves missing/unparseable output entries unchanged. |

The public interface and instance field layout are unchanged. Header comments were corrected for `isTerminal` and transferred child ownership. Safety fixes avoid the original signed 8-bit identifier-length overflow, delete arrays with `delete[]`, release partially parsed trees on syntax exceptions, preserve the iterator across vector reallocations, return null when an exhausted iterator is read, and write FILE identifiers as data rather than printf format strings. Exception-safe indentation scopes restore formatting after a stream failure. The unsupported state-zero direct encryption case is normalized to state 1 instead of indexing outside the table. The private syntax exception is thrown by value to avoid the original leaked exception allocation.

Tests were written before implementation and the parent observed `GraphicsScriptTests` fail against the reconstruction stub at `loadScript`. Tests use an independent byte fixture derived from the COFF table, alongside comments/quotes, nested searches, iteration, syntax errors, legacy numeric behavior, serialization layout, and ownership. An optional manifest argument loads every `ViewerScenes/*.ods` file without requiring a C++17 filesystem dependency.

Deliberately retained limitations: recursive parsing/lookup use the call stack; identifiers cannot contain literal quotes; encryption retains the original shared static state and is not thread safe; only one top-level statement is consumed, so trailing statements are left in the stream. These follow the original API/format rather than defining a replacement codec or grammar.

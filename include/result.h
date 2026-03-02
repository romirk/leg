//
// Created by Romir Kulshrestha on 12/11/2025.
//

#ifndef LEG_RESULT_H
#define LEG_RESULT_H

typedef struct {
    bool is_ok;
    void *value;
} Result;

#define Ok(v) (Result){.is_ok = true, .value = (void*)(v)}
#define Err(v) (Result){.is_ok = false, .value = (void*)(v)}

#define unwrap(result)     ((result).is_ok ? (result).value : panic("Expected Ok"))
#define unwrap_err(result) (!(result).is_ok ? (result).value : panic("Expected Err"))

#endif //LEG_RESULT_H

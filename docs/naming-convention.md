# Naming Convention

dcurl uses the **[snake case](https://en.wikipedia.org/wiki/Snake_case)** naming convention.

- Variable and function
  ```
  bool pow_c(void *pow_ctx) {
      ......
      int completed_index = -1; 
      ......
  }
  ```

- Structure: The suffixes **_s** represents for structure and **_t** represents for type
  ```
  typedef struct pwrok_s pwork_t;
  struct pwork_s {
      ......
  }
  ```

- Macro: Use capital letters
  ```
  #define MIN_TRYTE_VALUE -13
  #define MAX_TRYTE_VALUE 13
  ```


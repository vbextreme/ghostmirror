#ifndef __NOTSTD_CORE_REGEX_ERRORS_H__
#define __NOTSTD_CORE_REGEX_ERRORS_H__

#define TREX_ERR_EMPTY                   "empty regex"
#define TREX_ERR_ASPECTED_CLOSE_QNT      "ascpected quantifiers closed"
#define TREX_ERR_OVERFLOW_ARG            "arg overflow, arg value is to big"
#define TREX_ERR_UNKNOW_FLAG             "unknow regex flag"
#define TREX_ERR_UNTERMINATED_REGEX      "unterminated regex" 
#define TREX_ERR_UNTERMINATED_GROUP      "untermited group"
#define TREX_ERR_UNTERMINATED_GROUP_NAME "untermited group name"
#define TREX_ERR_UNTERMINATED_QUANTIFIER "unterminated quantifiers"
#define TREX_ERR_UNTERMINATED_SEQUENCES  "untermited sequences"
#define TREX_ERR_INVALID_SEQUENCE        "invalid sequence" 
#define TREX_ERR_INVALID_BACKREF         "unsupported backreferences inside sequences or quantifiers"
#define TREX_ERR_INVALID_NUMBERS         "invalid numbers"
#define TREX_ERR_INVALID_UNICODE         "invalid unicode value"
#define TREX_ERR_INVALID_QUANTIFIERS     "invalid quantifiers"
#define TREX_ERR_INVALID_REGEX           "invalid regex"
#define TREX_ERR_ASPECTED_CHARS          "aspected chars"
#define TREX_ERR_UNCONF_OPEN_NUM         "aspected { before numbers"
#define TREX_ERR_UNCONF_CLOSED_NUM       "aspected }"
#define TREX_ERR_UNASPECTED_QUANTIFIERS  "unaspected quantifiers"

#define TREX_ERR_VM_TO_MANY_THREADS      "vm call to many threads"
#define TREX_ERR_VM_TO_MANY_SAVE         "vm overflow save"

#endif

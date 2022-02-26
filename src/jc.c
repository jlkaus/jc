#include <stdio.h>   /* Standard input/output definitions */
#include <stdlib.h>  /* standard library stuff */
#include <stdint.h>
#include <string.h>  /* String function definitions */
#include <errno.h>   /* Error number definitions */
#include <argp.h>    /* arg_parse and co. */
#include <time.h>
#include <error.h>
#include <sysexits.h>

#define xstr(s) str(s)
#define str(s) #s

const char *argp_program_version = xstr(PKGNAME) " " xstr(VERSION);
const char *argp_program_bug_address = xstr(PKGMAINT);
static char program_documentation[] = xstr(PKGNAME) " - A better RPN calculator.\
\vIf no expressions or files are given, lines are read from standard input and \
executed as each line is entered.  This behavior can also be induced by \
specifying '-' as an input file.  Order between normal files/expressions and\
the placement of the standard input '-' is respected.  '-' can only be used once.\n\
Input expressions and input files are executed in the order they are specified \
on the command line.\n";

static char args_doc[] = "FILE ...";

static struct argp_option program_options[] = {
  {"verbose",           'v',0,          0,"Verbose mode."},
  {"expression",        'e',"EXPR",     0,"Add expression to the initial tape."},
  {"file",              'f',"FILE",     0,"Add contents of the file to add to the initial tape."},
  {"output",            'o',"FILE",     0,"Write output to FILE instead of standard output."},
  {"seed",              's',"SEED",     0,"Specify a particular starting seed for random numbers.  By default, a seed is generated from the current time."},
  {0,                   'h',0,          OPTION_HIDDEN, 0,-1},
  {0}
};

struct input_expr {
  const char *expression;
  const char *filename;
  struct input_expr *next;
};


struct program_arguments {
  int verbose;
  uint64_t seed;
  FILE *output_file;
  struct input_expr *expr_list;
  struct input_expr *last_expr;
};

void add_input_expr(struct program_arguments *args, const char *expr, const char *filename) {
  struct input_expr *cur_ptr = malloc(sizeof(struct input_expr));
  cur_ptr->expression = expr;
  cur_ptr->filename = filename;
  cur_ptr->next = NULL;

  if(!args->last_expr) {
    args->expr_list = args->last_expr = cur_ptr;
  } else {
    args->last_expr = args->last_expr->next = cur_ptr;
  }
}

void free_input_expr(struct input_expr *list) {
  if(list) {
    free_input_expr(list->next);
    free(list);
  }
}

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct program_arguments *args = state->input;

  switch(key) {
  case '?':
  case 'h':
    argp_state_help(state, stderr, ARGP_HELP_STD_HELP);
    break;
  case 'v':
    args->verbose = 1;
    break;
  case 'e':
    fprintf(stderr, "Got e arg [%s]\n", arg);
    add_input_expr(args, arg, NULL);
    break;
  case 'f':
    fprintf(stderr, "Got f arg [%s]\n", arg);
    add_input_expr(args, NULL, arg);
    break;
  case 'o':
    args->output_file = fopen(arg, "w");
    if(!args->output_file) {
      argp_failure(state, EX_CANTCREAT, errno, "Unable to open output file: %s", arg);
    }
    break;
  case 's':
    args->seed = strtoull(arg, NULL, 0);
    break;
  case ARGP_KEY_ARG:
    fprintf(stderr, "Got plain arg [%s]\n", arg);
    add_input_expr(args, NULL, arg);
    break;
  case ARGP_KEY_END:
    fprintf(stderr, "Got end arg\n");
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp program_arg_parser = {program_options, parse_opt, args_doc, program_documentation};

int main(int argc, char* argv[]) {
  program_invocation_name = xstr(PKGNAME);

  // interpret arguments
  struct program_arguments args = {0, (uint64_t)time(NULL), stdout, NULL, NULL};
  argp_parse(&program_arg_parser, argc, argv, ARGP_IN_ORDER, 0, &args);

  if(!args.expr_list) {
    // If no other input expressions or files are given, use stdin.
    add_input_expr(&args, NULL, "-");
  }

  // Set up other data structures...
  // Such as, the call stack, the data stack, macro refs, function refs, etc.

  struct input_expr *cur = args.expr_list;
  while(cur) {
    if(cur->expression) {
      fprintf(stderr, "Expression: [%s]\n", cur->expression);
      // parse and process the expression
    } else {
      fprintf(stderr, "Input file: [%s]\n", cur->filename);
      FILE* ifile = stdin;
      if(strcmp(cur->filename, "-") != 0) {
        ifile = fopen(cur->filename, "r");
        if(!ifile) {
          error(EX_NOINPUT, errno, "Unable to open input file: %s", cur->filename);
        }
      }
      // read/parse/process the lines in the given file.


      fclose(ifile);
    }

    cur = cur->next;
  }

  free_input_expr(args.expr_list);
  fclose(args.output_file);
  return 0;
}

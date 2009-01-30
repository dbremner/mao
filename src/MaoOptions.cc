//
// Copyright 2008 Google Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, 5th Floor, Boston, MA 02110-1301, USA.
#include <list>

#include <stdio.h>
#include <ctype.h>
#include "MaoDebug.h"
#include "MaoOptions.h"

const char *MaoOptions::assembly_output_file_name() {
  return assembly_output_file_name_;
}

const char *MaoOptions::ir_output_file_name() {
  return ir_output_file_name_;
}

void MaoOptions::set_assembly_output_file_name(const char *file_name) {
  MAO_ASSERT(file_name);
  write_assembly_ = true;
  output_is_stdout_ = false;
  assembly_output_file_name_ = file_name;
  if (verbose())
    fprintf(stderr, "asm output file: %s\n", file_name);
}

void MaoOptions::set_ir_output_file_name(const char *file_name) {
  MAO_ASSERT(file_name);
  write_ir_ = true;
  ir_output_file_name_ = file_name;
}


// Maintain mapping between option array and pass name.
//
class MaoOptionArray {
public:
  MaoOptionArray(const char *name, MaoOption *array, int num_entries)
    : name_(name), array_(array), num_entries_(num_entries) {
  }

  MaoOption  *FindOption(const char *option_name) {
    for (int i = 0; i < num_entries_; i++)
      if (!strcasecmp(option_name, array_[i].name))
        return &array_[i];
    MAO_ASSERT_MSG(false, "Option %s not found", option_name);
    return NULL;
  }

  const char *name() const { return name_; }

private:
  const char *name_;
  MaoOption  *array_;
  int         num_entries_;
};

// Maintain static list of all option arrays.
static std::list<MaoOptionArray *> option_array_list;

// Simple (static) constructor to allow registration of
// option arrays.
//
MaoOptionRegister::MaoOptionRegister(const char *name,
                                     MaoOption  *array,
                                     int N) {
  option_array_list.push_back(
    new MaoOptionArray(name, array, N) );
}


static MaoOptionArray *FindOptionArray(const char *pass_name) {
  for (std::list<MaoOptionArray *>::iterator it = option_array_list.begin();
       it != option_array_list.end(); ++it) {
    if (!strcasecmp((*it)->name(), pass_name))
      return (*it);
  }
  MAO_ASSERT_MSG(false, "Can't find passname: %s", pass_name);
  return NULL;
}

void MaoOptions::SetOption(const char *pass_name,
                           const char *option_name,
                           int         value) {
  MaoOptionArray *entry = FindOptionArray(pass_name);
  MaoOption      *option = entry->FindOption(option_name);
  MAO_ASSERT(option->type == OPT_INT);

  option->ival = value;
}

void MaoOptions::SetOption(const char *pass_name,
                           const char *option_name,
                           bool        value) {
  MaoOptionArray *entry = FindOptionArray(pass_name);
  MaoOption      *option = entry->FindOption(option_name);
  MAO_ASSERT(option->type == OPT_BOOL);

  option->bval = value;

}

void MaoOptions::SetOption(const char *pass_name,
                           const char *option_name,
                           const char *value) {
  MaoOptionArray *entry = FindOptionArray(pass_name);
  MaoOption      *option = entry->FindOption(option_name);
  MAO_ASSERT(option->type == OPT_STRING);

  option->cval = value;
}


static char token_buff[128];

static char *next_token(char *arg, char **next) {
  int i = 0;
  char *p = arg;
  if (*p == ',' || *p == ':' || *p == '=' || *p == '+')
    ++p;
  while (isalnum(*p) || (*p == '_') ||
         (*p == '/') ||
         (*p == '.') || (*p == '-'))
    token_buff[i++] = *p++;

  token_buff[i]='\0';
  *next=p;
  return token_buff;
}

static char *gobble_garbage(char *arg, char **next) {
  char *p = arg;
  if (*p == ',' || *p == ':' || *p == '=')
    ++p;
  *next = p;
  return p;
}


void MaoOptions::Parse(char *arg) {
  // Standard options?
  while (arg[0]) {
    gobble_garbage(arg, &arg);
    if (arg[0] == '-') {
      ++arg;
      if (arg[0] == 'v') {
        set_verbose();
        ++arg;
      } else
      if (arg[0] == 'h') {
        set_help();
        ++arg;
      } else
      if (arg[0] == 'o') {
        ++arg;
        char *filename;
        filename = next_token(arg, &arg);
        if (!strcmp(filename, "stderr")) {
          set_output_is_stderr();
          set_assembly_output_file_name("<stderr>");
        }
        else {
          set_assembly_output_file_name(filename);
        }
      } else {
        fprintf(stderr, "Invalid Option starting with: %s\n", arg);
        ++arg;
      }
      continue;
    }
    // Pass name?
    if (isascii(arg[0])) {
      char *pass = next_token(arg, &arg);

      if (FindOptionArray(pass)) {
        char *options;
        while (1) {
          options = next_token(arg, &arg);
          if (arg[0] != '+') break;
          if (arg[0] == '\0') break;
          ++arg;
        }
        gobble_garbage(arg, &arg);
      }
    }
    else {
      fprintf(stderr, "Unknown input: %s\n", arg);
      ++arg;
    }
  }
}

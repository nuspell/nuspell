/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * phonetic.c - generic replacement aglogithms for phonetic transformation
 * Copyright (C) 2000 Bjoern Jacke
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 * ***** END LICENSE BLOCK ***** */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "csutil.hxx"
#include "phonet.hxx"

void init_phonet_hash(phonetable& parms) {
  for (int i = 0; i < HASHSIZE; i++) {
    parms.hash[i] = -1;
  }

  for (int i = 0; parms.rules[i][0] != '\0'; i += 2) {
    /**  set hash value  **/
    int k = (unsigned char)parms.rules[i][0];

    if (parms.hash[k] < 0) {
      parms.hash[k] = i;
    }
  }
}

// like strcpy but safe if the strings overlap
//   but only if dest < src
static inline void strmove(char* dest, char* src) {
  while (*src)
    *dest++ = *src++;
  *dest = '\0';
}

static int myisalpha(char ch) {
  if ((unsigned char)ch < 128)
    return isalpha(ch);
  return 1;
}

/*  Do phonetic transformation.                        */
/*  phonetic transcription algorithm                   */
/*  see: http://aspell.net/man-html/Phonetic-Code.html */
/*  convert string to uppercase before this call       */
std::string phonet(const std::string& inword, phonetable& parms) {

  int i, k = 0, p, z;
  int k0, n0, p0 = -333;
  char c;
  typedef unsigned char uchar;

  size_t len = inword.size();
  if (len > MAXPHONETUTF8LEN)
    return std::string();
  char word[MAXPHONETUTF8LEN + 1];
  strncpy(word, inword.c_str(), MAXPHONETUTF8LEN);
  word[MAXPHONETUTF8LEN] = '\0';

  std::string target;
  /**  check word  **/
  i = z = 0;
  while ((c = word[i]) != '\0') {
    int n = parms.hash[(uchar)c];
    int z0 = 0;

    if (n >= 0 && !parms.rules[n].empty()) {
      /**  check all rules for the same letter  **/
      while (parms.rules[n][0] == c) {
        /**  check whole string  **/
        k = 1; /** number of found letters  **/
        p = 5; /** default priority  **/
        const char*s = parms.rules[n].c_str();
        s++; /**  important for (see below)  "*(s-1)"  **/

        while (*s != '\0' && word[i + k] == *s && !isdigit((unsigned char)*s) &&
               strchr("(-<^$", *s) == NULL) {
          k++;
          s++;
        }
        if (*s == '(') {
          /**  check letters in "(..)"  **/
          if (myisalpha(word[i + k])  // ...could be implied?
              && strchr(s + 1, word[i + k]) != NULL) {
            k++;
            while (*s != ')')
              s++;
            s++;
          }
        }
        p0 = (int)*s;
        k0 = k;
        while (*s == '-' && k > 1) {
          k--;
          s++;
        }
        if (*s == '<')
          s++;
        if (isdigit((unsigned char)*s)) {
          /**  determine priority  **/
          p = *s - '0';
          s++;
        }
        if (*s == '^' && *(s + 1) == '^')
          s++;

        if (*s == '\0' || (*s == '^' && (i == 0 || !myisalpha(word[i - 1])) &&
                           (*(s + 1) != '$' || (!myisalpha(word[i + k0])))) ||
            (*s == '$' && i > 0 && myisalpha(word[i - 1]) &&
             (!myisalpha(word[i + k0])))) {
          /**  search for followup rules, if:     **/
          /**  parms.followup and k > 1  and  NO '-' in searchstring **/
          char c0 = word[i + k - 1];
          n0 = parms.hash[(uchar)c0];

          //            if (parms.followup  &&  k > 1  &&  n0 >= 0
          if (k > 1 && n0 >= 0 && p0 != (int)'-' && word[i + k] != '\0' && !parms.rules[n0].empty()) {
            /**  test follow-up rule for "word[i+k]"  **/
            while (parms.rules[n0][0] == c0) {
              /**  check whole string  **/
              k0 = k;
              p0 = 5;
              s = parms.rules[n0].c_str();
              s++;
              while (*s != '\0' && word[i + k0] == *s &&
                     !isdigit((unsigned char)*s) &&
                     strchr("(-<^$", *s) == NULL) {
                k0++;
                s++;
              }
              if (*s == '(') {
                /**  check letters  **/
                if (myisalpha(word[i + k0]) &&
                    strchr(s + 1, word[i + k0]) != NULL) {
                  k0++;
                  while (*s != ')' && *s != '\0')
                    s++;
                  if (*s == ')')
                    s++;
                }
              }
              while (*s == '-') {
                /**  "k0" gets NOT reduced   **/
                /**  because "if (k0 == k)"  **/
                s++;
              }
              if (*s == '<')
                s++;
              if (isdigit((unsigned char)*s)) {
                p0 = *s - '0';
                s++;
              }

              if (*s == '\0'
                  /**  *s == '^' cuts  **/
                  || (*s == '$' && !myisalpha(word[i + k0]))) {
                if (k0 == k) {
                  /**  this is just a piece of the string  **/
                  n0 += 2;
                  continue;
                }

                if (p0 < p) {
                  /**  priority too low  **/
                  n0 += 2;
                  continue;
                }
                /**  rule fits; stop search  **/
                break;
              }
              n0 += 2;
            } /**  End of "while (parms.rules[n0][0] == c0)"  **/

            if (p0 >= p && parms.rules[n0][0] == c0) {
              n += 2;
              continue;
            }
          } /** end of follow-up stuff **/

          /**  replace string  **/
          s = parms.rules[n + 1].c_str();
          p0 = (!parms.rules[n].empty() &&
                strchr(parms.rules[n].c_str() + 1, '<') != NULL)
                   ? 1
                   : 0;
          if (p0 == 1 && z == 0) {
            /**  rule with '<' is used  **/
            if (!target.empty() && *s != '\0' &&
                (target[target.size()-1] == c || target[target.size()-1] == *s)) {
              target.erase(target.size() - 1);
            }
            z0 = 1;
            z = 1;
            k0 = 0;
            while (*s != '\0' && word[i + k0] != '\0') {
              word[i + k0] = *s;
              k0++;
              s++;
            }
            if (k > k0)
              strmove(&word[0] + i + k0, &word[0] + i + k);

            /**  new "actual letter"  **/
            c = word[i];
          } else { /** no '<' rule used **/
            i += k - 1;
            z = 0;
            while (*s != '\0' && *(s + 1) != '\0' && target.size() < len) {
              if (target.empty() || target[target.size()-1] != *s) {
                target.push_back(*s);
              }
              s++;
            }
            /**  new "actual letter"  **/
            c = *s;
            if (!parms.rules[n].empty() &&
                strstr(parms.rules[n].c_str() + 1, "^^") != NULL) {
              if (c != '\0') {
                target.push_back(c);
              }
              strmove(&word[0], &word[0] + i + 1);
              i = 0;
              z0 = 1;
            }
          }
          break;
        } /** end of follow-up stuff **/
        n += 2;
      } /**  end of while (parms.rules[n][0] == c)  **/
    }   /**  end of if (n >= 0)  **/
    if (z0 == 0) {
      if (k && !p0 && target.size() < len && c != '\0') {
        /**  condense only double letters  **/
        target.push_back(c);
        /// printf("\n setting \n");
      }

      i++;
      z = 0;
      k = 0;
    }
  } /**  end of   while ((c = word[i]) != '\0')  **/

  return target;
} /**  end of function "phonet"  **/

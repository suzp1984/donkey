;;; my-auto-insert.el --- auto insert configure

;; Copyright (C) 2013  zxsu

;; Author: zxsu <suzp1984@gmail.com>
;; Keywords: 

;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;;; Commentary:

;; 

;;; Code:

(require 'autoinsert)

(defun my/autoinsert-yas-expand ()
  "Replace text in yasnippet template."
  (yas-expand-snippet (buffer-string) (point-min) (point-max)))

(custom-set-variables
 '(auto-insert 'other)
 '(auto-insert-directory "~/insert/")
 )

(add-to-list 'auto-insert-alist '(("\\.py\\'" . "Python souce code header") 
                                  ["template.py" my/autoinsert-yas-expand]))

(add-to-list 'auto-insert-alist '(("\\.\\([Hh]\\|hh\\|hpp\\)\\'" . "C / C++ header") . 
                                  ["template.h" my/autoinsert-yas-expand]))

(add-to-list 'auto-insert-alist '(("\\.\\([Cc]\\|cc\\|cpp\\)\\'" . "C / C++ program") 
                                  ["template.c" my/autoinsert-yas-expand]))

(provide 'my-auto-insert)
;;; my-auto-insert.el ends here

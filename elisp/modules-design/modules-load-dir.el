;;; modules-load-dir.el --- auto load all packages in peticular directory

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

(defvar modules-default-directory "./elisp/")

(add-to-list 'load-path (expand-file-name modules-default-directory))

(let* ((directory modules-default-directory)
       (files (directory-files directory nil "^my-.*.el$" nil)))
  (mapc (lambda (name) (require (intern-soft (file-name-sans-extension name))))
        files))

(provide 'modules-load-dir)
;;; modules-load-dir.el ends here

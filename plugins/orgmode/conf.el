;; Load htmlize.el
(let ((conf (expand-file-name "htmlize.el" (file-name-directory load-file-name))))
  (if (file-exists-p conf)
      (load-file conf)))

(require 'ox-html)
(org-babel-do-load-languages
 'org-babel-load-languages
 '((dot . t))) ; this line activates dot
(defun my-org-confirm-babel-evaluate (lang body)
  (not (string= lang "dot")))  ; don't ask for ditaa
(setq org-confirm-babel-evaluate 'my-org-confirm-babel-evaluate)
(defun my-html-filter-files (text backend info)
  "Fix image path"
  (when (org-export-derived-backend-p backend 'html)
    (replace-regexp-in-string "../files/" "../" text)
    ))

(add-to-list 'org-export-filter-link-functions
             'my-html-filter-files)

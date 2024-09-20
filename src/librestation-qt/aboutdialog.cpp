// SPDX-FileCopyrightText: 2019-2023 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "aboutdialog.h"
#include "qtutils.h"

#include "core/settings.h"

#include "common/file_system.h"
#include "common/path.h"

#include "scmversion/scmversion.h"

#include <QtCore/QString>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextBrowser>

AboutDialog::AboutDialog(QWidget* parent /* = nullptr */) : QDialog(parent)
{
  m_ui.setupUi(this);

  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setFixedSize(geometry().width(), geometry().height());

  m_ui.scmversion->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_ui.scmversion->setText(
    tr("%1 (%2)").arg(QLatin1StringView(g_scm_tag_str)).arg(QLatin1StringView(g_scm_branch_str)));

  m_ui.description->setTextInteractionFlags(Qt::TextBrowserInteraction);
  m_ui.description->setOpenExternalLinks(true);
  m_ui.description->setText(QStringLiteral(R"(
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd">
<html><head><meta name="qrichtext" content="1" /><style type="text/css">
p, li { white-space: pre-wrap; }
</style></head><body style=" font-size:10pt; font-weight:400; font-style:normal;">
<p style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">%1</p>
<p style=" margin-top:12px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style=" font-weight:600;">%2</span>:</p>
<p style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">  DuckStation and LibreStation contributors</p>
<p style=" margin-top:12px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style=" font-weight:600;">%3</span>:</p>
<p style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">  GPL-3.0-only</p>
<p style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"> </p>
<p style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"> </p>
</body></html>
)")
                              .arg(tr("LibreStation is a free and open-source emulator of the Sony "
                                      "PlayStation<span style=\"vertical-align:super;\">TM</span> console, focusing on "
                                      "playability, speed, and long-term maintainability."))
                              .arg(tr("Authors"))
                              .arg(tr("License")));
}

AboutDialog::~AboutDialog() = default;

void AboutDialog::showThirdPartyNotices(QWidget* parent)
{
  QDialog dialog(parent);
  dialog.setMinimumSize(700, 400);
  dialog.setWindowTitle(tr("LibreStation Third-Party Notices"));

  QIcon icon;
  icon.addFile(QString::fromUtf8(":/icons/duck.png"), QSize(), QIcon::Normal, QIcon::Off);
  dialog.setWindowIcon(icon);

  QVBoxLayout* layout = new QVBoxLayout(&dialog);

  QTextBrowser* tb = new QTextBrowser(&dialog);
  tb->setAcceptRichText(true);
  tb->setReadOnly(true);
  tb->setOpenExternalLinks(true);
  if (std::optional<std::string> notice =
        FileSystem::ReadFileToString(Path::Combine(EmuFolders::Resources, "thirdparty.html").c_str());
      notice.has_value())
  {
    tb->setText(QString::fromStdString(notice.value()));
  }
  else
  {
    tb->setText(tr("Missing thirdparty.html file. You should request it from where-ever you obtained LibreStation."));
  }
  layout->addWidget(tb, 1);

  QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
  connect(bb->button(QDialogButtonBox::Close), &QPushButton::clicked, &dialog, &QDialog::done);
  layout->addWidget(bb, 0);

  dialog.exec();
}

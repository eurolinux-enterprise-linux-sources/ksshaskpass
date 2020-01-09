/***************************************************************************
 *   Copyright (C) 2006 Hans van Leeuwen <hanz@hanz.nl>                    *
 *   Copyright (C) 2008 Armin Berres <trigger@space-based.de>              *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include <sys/resource.h>

#include <kwallet.h>
#include <KPasswordDialog>
#include <KApplication>
#include <KAboutData>
#include <KCmdLineArgs>
#include <KLocalizedString>
#include <QTextStream>

int main(int argc, char **argv)
{
  KAboutData about (
    "ksshaskpass",
    0,
    ki18n("Ksshaskpass"),
    "0.5.0",
    ki18n("KDE version of ssh-askpass"),
    KAboutData::License_GPL,
    ki18n("(c) 2006 Hans van Leeuwen\n(c) 2008 Armin Berres"),
    ki18n("Ksshaskpass allows you to interactively prompt users for a passphrase for ssh-add"),
    "http://www.kde-apps.org/content/edit.php?content=50971",
    "trigger@space-based.de"
  );

  about.addAuthor(ki18n("Armin Berres"), ki18n("Current author"), "trigger@space-based.de", 0);
  about.addAuthor(ki18n("Hans van Leeuwen"), ki18n("Original author"), "hanz@hanz.nl", 0);

  KCmdLineOptions options;
  options.add("+[prompt]",ki18n("Prompt")); 
  KCmdLineArgs::init(argc, argv, &about);
  KCmdLineArgs::addCmdLineOptions( options );
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  KApplication app;

  // Disable Session Management. We don't need it.
  app.disableSessionManagement();

  QString walletFolder = about.appName();
  QString dialog = i18n("Please enter password");  // Default dialog text.
  QString keyFile;
  QString password;

  // Parse commandline arguments
  if ( args->count() > 0 ) {
    dialog = args->arg(0);
    keyFile = dialog.section(" ", -2).remove(":");
  }
  args->clear();

  // Open KWallet to see if a password was previously stored.
  // TODO: KWallet wants a valid window here, but we don't have a window so far. What to do?
  KWallet::Wallet *wallet = KWallet::Wallet::openWallet( KWallet::Wallet::NetworkWallet(), 0 );

  if ( wallet && wallet->hasFolder(walletFolder) ) {
    wallet->setFolder(walletFolder);

    QString retrievedPass;

    wallet->readPassword(keyFile, retrievedPass);

    if ( !retrievedPass.isEmpty() ) {
      password = retrievedPass;
    }
  }

  // Password could not be retrieved from wallet. Open password dialog
  if ( password.isEmpty() ) {
    // create the password dialog, but only show "Enable Keep" button, if the wallet is open
    KPasswordDialog::KPasswordDialogFlag flag;
    if ( wallet ) {
      flag = KPasswordDialog::ShowKeepPassword;
    }
    KPasswordDialog kpd(0, flag);

    kpd.setPrompt(dialog);
    kpd.setCaption(i18n("Ksshaskpass"));
    // We don't want to dump core when the password dialog is shown, because it could contain the entered password.
    // KPasswordDialog::disableCoreDumps() seems to be gone in KDE 4 -- do it manually
    struct rlimit rlim;
    rlim.rlim_cur = rlim.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &rlim);

    if ( kpd.exec() == KDialog::Accepted ) {
      password = kpd.password();
      // If "Enable Keep" is enabled, open/create a folder in KWallet and store the password.
      if ( wallet && kpd.keepPassword() ) {
        if ( !wallet->hasFolder( walletFolder ) ) {
          wallet->createFolder(walletFolder);
        }
        wallet->setFolder(walletFolder);
        wallet->writePassword(keyFile, password);
      }
    } else
    {
      // dialog has been cancelled
      return 1;
    }
  }

  delete wallet;

  QTextStream out(stdout);
  out << password;
  return 0; 
}

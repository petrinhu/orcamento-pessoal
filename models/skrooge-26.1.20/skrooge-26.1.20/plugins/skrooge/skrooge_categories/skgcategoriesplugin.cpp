/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin to generate categories.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgcategoriesplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <kselectaction.h>

#include <qaction.h>
#include <qdiriterator.h>
#include <qfileinfo.h>
#include <qstandardpaths.h>

#include "skgcategoriespluginwidget.h"
#include "skgcategoryobject.h"
#include "skgdocumentbank.h"
#include "skghtmlboardwidget.h"
#include "skgimportexportmanager.h"
#include "skgmainpanel.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGCategoriesPlugin, "metadata.json")

SKGCategoriesPlugin::SKGCategoriesPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentBankDocument(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGCategoriesPlugin::~SKGCategoriesPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentBankDocument = nullptr;
}

bool SKGCategoriesPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentBankDocument = qobject_cast<SKGDocumentBank *>(iDocument);
    if (m_currentBankDocument == nullptr) {
        return false;
    }

    setComponentName(QStringLiteral("skrooge_categories"), title());
    setXMLFile(QStringLiteral("skrooge_categories.rc"));

    // Import categories
    QStringList overlaycategories;
    overlaycategories.push_back(icon());

    QStringList overlaydelete;
    overlaydelete.push_back(QStringLiteral("edit-delete"));

    auto contextMenu =
        new KSelectAction(SKGServices::fromTheme(QStringLiteral("document-import"), overlaycategories), i18nc("Verb", "Import categories"), this);
    registerGlobalAction(QStringLiteral("import_categories"), contextMenu);

    QAction *actImportStdCat =
        contextMenu->addAction(SKGServices::fromTheme(QStringLiteral("document-import"), overlaycategories), i18nc("Verb", "Import standard categories"));
    actImportStdCat->setCheckable(false);
    connect(actImportStdCat, &QAction::triggered, this, &SKGCategoriesPlugin::importStandardCategories);
    registerGlobalAction(QStringLiteral("import_standard_categories"), actImportStdCat);

    const auto dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                "skrooge/categories/" % QLocale().name().split(QStringLiteral("_")).at(0),
                                                QStandardPaths::LocateDirectory);
    for (const auto &dir : dirs) {
        QDirIterator it(dir, QStringList() << QStringLiteral("*.qif"));
        while (it.hasNext()) {
            QString cat = it.next();

            QString name = QFileInfo(cat).baseName().replace('_', ' ');
            QAction *act = contextMenu->addAction(SKGServices::fromTheme(QStringLiteral("document-import"), overlaycategories),
                                                  i18nc("Verb", "Import categories [%1]", name));
            act->setCheckable(false);
            act->setData(cat);
            connect(act, &QAction::triggered, this, &SKGCategoriesPlugin::importCategories);
            registerGlobalAction("import_categories_" % name, act);
        }
    }

    auto deleteUnusedCategoriesAction = new QAction(SKGServices::fromTheme(icon(), overlaydelete), i18nc("Verb", "Delete unused categories"), this);
    connect(deleteUnusedCategoriesAction, &QAction::triggered, this, &SKGCategoriesPlugin::deleteUnusedCategories);
    registerGlobalAction(QStringLiteral("clean_delete_unused_categories"), deleteUnusedCategoriesAction);

    // ------------
    auto actTmp = new QAction(SKGServices::fromTheme(icon()), i18nc("Verb", "Open similar categories…"), this);
    actTmp->setData(
        QString("skg://skrooge_categories_plugin/?title_icon=" % icon() % "&title="
                % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Similar categories")) % "&whereClause="
                % SKGServices::encodeForUrl(QStringLiteral("id IN (SELECT category.id FROM category, ("
                                                           "SELECT * FROM category C WHERE EXISTS (SELECT 1 FROM category p2 WHERE p2.id<>C.id AND "
                                                           "upper(p2.t_fullname)=upper(C.t_fullname) AND p2.t_fullname<>C.t_fullname)) "
                                                           "AS C WHERE category.t_fullname=C.t_fullname OR C.t_fullname LIKE category.t_fullname||' > %')"))));
    connect(actTmp, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    registerGlobalAction(QStringLiteral("view_open_similar_categories"), actTmp);
    /*TODO
    WITH RECURSIVE
    cat(t_fullname,id, level) AS (
    SELECT t_fullname, id, 0 FROM v_category_display WHERE EXISTS (SELECT 1 FROM category p2 WHERE p2.id<>v_category_display.id AND
    upper(p2.t_fullname)=upper(v_category_display.t_fullname) AND p2.t_fullname<>v_category_display.t_fullname) UNION ALL SELECT v_category_display.t_name,
    v_category_display.id, cat.level+1 FROM v_category_display JOIN cat ON v_category_display.rd_category_id=cat.id ORDER BY 2 ) SELECT substr('……….',1,level*3)
    || t_fullname FROM cat;
    */
    return true;
}

int SKGCategoriesPlugin::getNbDashboardWidgets()
{
    SKGTRACEINFUNC(1)
    return 4;
}

QString SKGCategoriesPlugin::getDashboardWidgetTitle(int iIndex)
{
    SKGTRACEINFUNC(1)
    if (iIndex == 0) {
        return i18nc("Report header", "5 main categories of expenditure");
    }
    if (iIndex == 1) {
        return i18nc("Report header", "5 main variations");
    }
    if (iIndex == 2) {
        return i18nc("Report header", "Budget");
    }
    return i18nc("Report header", "5 main variations (issues)");
}

SKGBoardWidget *SKGCategoriesPlugin::getDashboardWidget(int iIndex)
{
    SKGTRACEINFUNC(1)
    if (iIndex == 0) {
        return new SKGHtmlBoardWidget(
            SKGMainPanel::getMainPanel(),
            m_currentBankDocument,
            getDashboardWidgetTitle(iIndex),
            QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/categories_period_table.qml")),
            QStringList() << QStringLiteral("v_suboperation_consolidated"),
            SKGSimplePeriodEdit::ALL_PERIODS);
    }
    if (iIndex == 1) {
        return new SKGHtmlBoardWidget(
            SKGMainPanel::getMainPanel(),
            m_currentBankDocument,
            getDashboardWidgetTitle(iIndex) % " - %1",
            QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/categories_variations.qml")),
            QStringList() << QStringLiteral("v_suboperation_consolidated"),
            SKGSimplePeriodEdit::PREVIOUS_AND_CURRENT_PERIODS);
    }
    if (iIndex == 2) {
        return new SKGHtmlBoardWidget(SKGMainPanel::getMainPanel(),
                                      m_currentBankDocument,
                                      getDashboardWidgetTitle(iIndex) % " - %1",
                                      QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/budget_table.qml")),
                                      QStringList() << QStringLiteral("v_budget"),
                                      SKGSimplePeriodEdit::PREVIOUS_AND_CURRENT_MONTHS);
    }
    return new SKGHtmlBoardWidget(
        SKGMainPanel::getMainPanel(),
        m_currentBankDocument,
        getDashboardWidgetTitle(iIndex) % " - %1",
        QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/categories_variations_issues.qml")),
        QStringList() << QStringLiteral("v_suboperation_consolidated"),
        SKGSimplePeriodEdit::PREVIOUS_AND_CURRENT_PERIODS);
}

void SKGCategoriesPlugin::refresh()
{
    SKGTRACEINFUNC(10)
    if (m_currentBankDocument != nullptr) {
        // Automatic categories creation
        if (m_currentBankDocument->getMainDatabase() != nullptr) {
            QString doc_id = m_currentBankDocument->getUniqueIdentifier();
            if (m_docUniqueIdentifier != doc_id) {
                m_docUniqueIdentifier = std::move(doc_id);

                bool exist = false;
                SKGError err = m_currentBankDocument->existObjects(QStringLiteral("category"), QString(), exist);
                if (!err && !exist) {
                    importStandardCategories();

                    // The file is considered has not modified
                    m_currentBankDocument->setFileNotModified();
                }
            }
        }
    }
}

void SKGCategoriesPlugin::importCategories()
{
    SKGTRACEINFUNC(10)
    SKGError err;
    auto *act = qobject_cast<QAction *>(sender());
    if (act != nullptr) {
        QString fileName = act->data().toString();
        QString name = QFileInfo(fileName).baseName().replace('_', ' ');
        {
            SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Verb", "Import categories [%1]", name), err)

            SKGImportExportManager imp(m_currentBankDocument, QUrl(fileName));
            err = imp.importFile();
            IFOKDO(err, m_currentBankDocument->removeMessages(m_currentBankDocument->getCurrentTransaction()))
        }

        // status
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Categories imported.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Importing categories failed."));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGCategoriesPlugin::importStandardCategories()
{
    SKGTRACEINFUNC(10)
    SKGError err;
    {
        QString cats = i18nc(
            "List of categories. It is not needed to translate each item. You can set the list you want. ';' must be used to separate categories. ' > ' must "
            "be used to separate category and sub category (no limit of level).",
            "Alimony;Auto;Auto > Fuel;Auto > Insurance;Auto > Lease;Auto > Loan;Auto > Registration;Auto > Service;Bank Charges;Bank Charges > Interest "
            "Paid;Bank Charges > Service Charge;Bills;Bills > Electricity;"
            "Bills > Fuel Oil;Bills > Local Taxes;Bills > Mortgage;Bills > Natural Gas;Bills > Rent;Bills > TV;Bills > Telephone;Bills > Water & "
            "Sewage;Bonus;Business;Business > Auto;Business > Capital Goods;Business > Legal Expenses;Business > Office Rent;"
            "Business > Office Supplies;Business > Other;Business > Revenue;Business > Taxes;Business > Travel;Business > Utilities;Business > Wages & "
            "Salary;Car;Car > Fuel;Car > Insurance;Car > Lease;Car > Loan;Car > Registration;Car > Service;"
            "Cash Withdrawal;Charity;Charity > Donations;Child Care;Child Support;Clothing;Disability;Div Income;Div Income > Ord dividend;Div Income > Stock "
            "dividend;Education;Education > Board;Education > Books;Education > Fees;Education > Loans;"
            "Education > Tuition;Employment;Employment > Benefits;Employment > Foreign;Employment > Lump sums;Employment > Other employ;Employment > Salary & "
            "wages;Food;Food > Dining Out;Food > Groceries;Gardening;"
            "Gift Received;Gifts;Healthcare;Healthcare > Dental;Healthcare > Doctor;Healthcare > Hospital;Healthcare > Optician;Healthcare > "
            "Prescriptions;Holidays;Holidays > Accommodation;Holidays > Travel;Household;"
            "Household > Furnishings;Household > Repairs;Insurance;Insurance > Auto;Insurance > Disability;Insurance > Home and Contents;Insurance > "
            "Life;Insurance > Medical;Int Inc;Int Inc > Bank Interest;Int Inc > Gross;Int Inc > Net;"
            "Int Inc > Other savings;Invest. income;Invest. income > 1st option;Invest. income > Dividend;Invest. income > Foreign;Invest. income > Other "
            "savings;Invest. income > Other trusts;Invest. income > Other trusts#Capital;"
            "Invest. income > Other trusts#Dist. rec'd;Invest. income > Other trusts#Estate;Investment Income;Investment Income > Dividends;Investment Income "
            "> Interest;Investment Income > Long-Term Capital Gains;"
            "Investment Income > Short-Term Capital Gains;Investment Income > Tax-Exempt Interest;Job Expense;Job Expense > Non-Reimbursed;Job Expense > "
            "Reimbursed;Legal Fees;Leisure;Leisure > Books & Magazines;Leisure > Entertaining;"
            "Leisure > Films & Video Rentals;Leisure > Hobbies;Leisure > Sporting Events;Leisure > Sports Goods;Leisure > Tapes & CDs;Leisure > Theatre & "
            "Concerts etc;Leisure > Toys & Games;Loan;Loan > Loan Interest;Long-Term Capital gains;Mortgage;Mortgage > Interest;Mortgage > PMI;Mortgage > "
            "Principle;Motor;Motor > Fuel;Motor > Loan;Motor > Service;Other Expense;Other Expense > Unknown;Other Income;Other Income > Child Support;"
            "Other Income > Employee Share Option;Other Income > Gifts Received;Other Income > Loan Principal Received;Other Income > Lottery or Premium Bond "
            "Prizes;Other Income > Student loan;Other Income > Tax Refund;"
            "Other Income > Unemployment Benefit;Pension;Pension > Employer;Personal Care;Pet Care;Pet Care > Food;Pet Care > Supplies;Pet Care > Vet's "
            "Bills;Recreation;Retirement Accounts;Retirement Accounts > 401(k)403(b) Plan Contributions;"
            "Retirement Accounts > 529 Plan Contributions;Retirement Accounts > IRA Contributions;Retirement Income;Retirement Income > 401(k);Retirement "
            "Income > 401(k) > 403(b) Distributions;Retirement Income > IRA Distributions;"
            "Retirement Income > Pensions & Annuities;Retirement Income > State Pension Benefits;Short-Term Capital gains;Social Security Benefits;Taxes;Taxes "
            "> AMT;Taxes > Federal Tax;Taxes > Federal Taxes;Taxes > Local Tax;Taxes > Local Taxes;"
            "Taxes > Other Invest;Taxes > Other Tax;Taxes > Property Taxes;Taxes > Social Security;Taxes > State Tax;Taxes > State Taxes;Travel;Travel > "
            "Accommodations;Travel > Car Rental;Travel > Fares;Utilities;Utilities > Electricity;"
            "Utilities > Garbage & Recycling;Utilities > Gas;Utilities > Sewer;Utilities > Telephone;Utilities > Water;Wages & Salary;Wages & Salary > "
            "Benefits;Wages & Salary > Bonus;Wages & Salary > Commission;"
            "Wages & Salary > Employer Pension Contributions;Wages & Salary > Gross Pay;Wages & Salary > Net Pay;Wages & Salary > Overtime;Wages & Salary > "
            "Workman's Comp");

        SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Import standard categories"), err)

        const auto items = SKGServices::splitCSVLine(cats, ';');
        for (const auto &item : items) {
            QString line = item.trimmed();
            if (!line.isEmpty()) {
                SKGCategoryObject cat;
                err = SKGCategoryObject::createPathCategory(m_currentBankDocument, line, cat);
            }
        }
    }

    // status
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Categories imported.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Importing categories failed."));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

SKGTabPage *SKGCategoriesPlugin::getWidget()
{
    SKGTRACEINFUNC(10)
    return new SKGCategoriesPluginWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
}

QString SKGCategoriesPlugin::title() const
{
    return i18nc("Noun, categories of items", "Categories");
}

QString SKGCategoriesPlugin::icon() const
{
    return QStringLiteral("applications-all-symbolic");
}

QString SKGCategoriesPlugin::toolTip() const
{
    return i18nc("A tool tip", "Categories management");
}

QStringList SKGCategoriesPlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tips", "<p>… <a href=\"skg://skrooge_categories_plugin\">categories</a> can be reorganized by drag & drop.</p>"));
    output.push_back(i18nc("Description of a tips",
                           "<p>… if you delete a <a href=\"skg://skrooge_categories_plugin\">category</a>, all transactions affected by this category will be "
                           "associated to its parent category.</p>"));
    return output;
}

int SKGCategoriesPlugin::getOrder() const
{
    return 30;
}

bool SKGCategoriesPlugin::isInPagesChooser() const
{
    return true;
}

SKGAdviceList SKGCategoriesPlugin::advice(const QStringList &iIgnoredAdvice)
{
    SKGTRACEINFUNC(10)
    SKGAdviceList output;
    // Check unused categories
    if (!iIgnoredAdvice.contains(QStringLiteral("skgcategoriesplugin_unused"))) {
        bool exist = false;
        m_currentBankDocument->existObjects(QStringLiteral("v_category_used2"), QStringLiteral("t_ISUSEDCASCADE='N'"), exist);
        if (exist) {
            SKGAdvice ad;
            ad.setUUID(QStringLiteral("skgcategoriesplugin_unused"));
            ad.setPriority(5);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Many unused categories"));
            ad.setLongMessage(i18nc("Advice on making the best (long)", "You can improve performances by removing categories that have no transactions."));
            SKGAdvice::SKGAdviceActionList autoCorrections;
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = QStringLiteral("skg://clean_delete_unused_categories");
                a.IsRecommended = true;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    // Check transactions not validated
    if (!iIgnoredAdvice.contains(QStringLiteral("skgmonthlyplugin_maincategoriesvariation"))) {
        QString month = QDate::currentDate().toString(QStringLiteral("yyyy-MM"));
        QDate datepreviousmonth = QDate::currentDate().addDays(-QDate::currentDate().day());
        QString previousmonth = datepreviousmonth.toString(QStringLiteral("yyyy-MM"));

        QStringList listCategories;
        QStringList listVariations = m_currentBankDocument->get5MainCategoriesVariationList(month, previousmonth, true, &listCategories);

        int nb = listVariations.count();
        SKGAdvice::SKGAdviceActionList autoCorrections;
        for (int i = 0; i < nb; ++i) {
            SKGAdvice ad;
            ad.setUUID("skgmonthlyplugin_maincategoriesvariation|" % listCategories.at(i));
            ad.setPriority(7);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Important variation for '%1'", listCategories.at(i)));
            ad.setLongMessage(listVariations.at(i));
            autoCorrections.resize(0);
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Advice on making the best (action)", "Open sub transactions with category containing '%1'", listCategories.at(i));
                a.IconName = QStringLiteral("quickopen");
                a.IsRecommended = false;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    // Check categories with different case
    if (!iIgnoredAdvice.contains(QStringLiteral("skgcategoriesplugin_case"))) {
        bool exist = false;
        m_currentBankDocument->existObjects(QStringLiteral("category"),
                                            QStringLiteral("EXISTS (SELECT 1 FROM category p2 WHERE p2.id<>category.id AND "
                                                           "upper(p2.t_fullname)=upper(category.t_fullname) AND p2.t_fullname<>category.t_fullname)"),
                                            exist);
        if (exist) {
            SKGAdvice ad;
            ad.setUUID(QStringLiteral("skgcategoriesplugin_case"));
            ad.setPriority(3);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Some categories seem to be identical"));
            ad.setLongMessage(
                i18nc("Advice on making the best (long)", "Some categories seem to be identical but with different syntax. They could be merged."));
            SKGAdvice::SKGAdviceActionList autoCorrections;
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = QStringLiteral("skg://view_open_similar_categories");
                a.IsRecommended = false;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }
    return output;
}

SKGError SKGCategoriesPlugin::executeAdviceCorrection(const QString &iAdviceIdentifier, int iSolution)
{
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgmonthlyplugin_maincategoriesvariation|"))) {
        // Get parameters
        QString category = iAdviceIdentifier.right(iAdviceIdentifier.length() - 41);
        QString month = QDate::currentDate().toString(QStringLiteral("yyyy-MM"));

        // Call transaction plugin
        SKGMainPanel::getMainPanel()->openPage(
            "skg://skrooge_operation_plugin/SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS/?currentPage=-1&title_icon=" % icon()
            % "&operationTable=v_suboperation_consolidated&title="
            % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Sub transactions with category containing '%1'", category)) % "&operationWhereClause="
            % SKGServices::encodeForUrl("d_DATEMONTH='" % month % "' AND t_REALCATEGORY='" % SKGServices::stringToSqlString(category) % u'\''));
        return SKGError();
    }
    return SKGInterfacePlugin::executeAdviceCorrection(iAdviceIdentifier, iSolution);
}

void SKGCategoriesPlugin::deleteUnusedCategories() const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    if (m_currentBankDocument != nullptr) {
        SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Delete unused categories"), err)

        QStringList categoriesUsed;
        err = m_currentBankDocument->getDistinctValues(
            QStringLiteral("category"),
            QStringLiteral("t_fullname"),
            QStringLiteral("t_fullname in ("
                           "SELECT DISTINCT(category.t_fullname) FROM category, suboperation WHERE suboperation.r_category_id=category.id UNION ALL "
                           "SELECT DISTINCT(category.t_fullname) FROM category, budget WHERE budget.rc_category_id=category.id UNION ALL "
                           "SELECT DISTINCT(category.t_fullname) FROM category, budgetrule WHERE budgetrule.rc_category_id=category.id UNION ALL "
                           "SELECT DISTINCT(category.t_fullname) FROM category, budgetrule WHERE budgetrule.rc_category_id_target=category.id)"),
            categoriesUsed);

        for (int i = 0; i < categoriesUsed.count(); ++i) { // Warning categoriesUsed is modified in the loop
            QString cat = categoriesUsed.at(i);
            categoriesUsed[i] = SKGServices::stringToSqlString(cat);
            int pos = cat.lastIndexOf(OBJECTSEPARATOR);
            if (pos != -1) {
                categoriesUsed.push_back(cat.left(pos));
            }
        }

        IFOK(err)
        {
            QString sql;
            if (!categoriesUsed.isEmpty()) {
                sql = "DELETE FROM category WHERE t_fullname NOT IN ('" % categoriesUsed.join(QStringLiteral("','")) % "')";
            } else {
                sql = QStringLiteral("DELETE FROM category");
            }
            err = m_currentBankDocument->executeSqliteOrder(sql);
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Unused categories deleted")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Unused categories deletion failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

#include <skgcategoriesplugin.moc>

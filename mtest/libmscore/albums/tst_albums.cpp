//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2019 MuseScore BVBA
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include <QtTest/QtTest>
#include "mtest/testutils.h"
#include "mscore/musescore.h"
#include "mscore/preferences.h"
#include "libmscore/measure.h"

#define DIR QString("libmscore/albums/")

using namespace Ms;

//---------------------------------------------------------
//   TestAlbums
//---------------------------------------------------------

class TestAlbums : public QObject, public MTest
{
    Q_OBJECT

    void albumAddScore();
    void albumItemDuration();
    void albumItemEnable();
    void albumItemBreaks();
    void albumBreaks();
    void realTitleNoCrash();
    void albumScoreTitlesNoCrash();
    void activeAlbumClearedOnDestruction();
    void addScoreTwiceRejected();

private slots:
    void initTestCase();

    void albumAddScoreTest() { albumAddScore(); }
    void albumItemDurationTest() { albumItemDuration(); }
    void albumItemEnableTest() { albumItemEnable(); }
    void albumItemBreaksTest() { albumItemBreaks(); }
    void albumBreaksTest() { albumBreaks(); }
    void realTitleNoCrashTest() { realTitleNoCrash(); }
    void albumScoreTitlesNoCrashTest() { albumScoreTitlesNoCrash(); }
    void activeAlbumClearedOnDestructionTest() { activeAlbumClearedOnDestruction(); }
    void addScoreTwiceRejectedTest() { addScoreTwiceRejected(); }
};

#define private public
#include "libmscore/album.h"

//---------------------------------------------------------
//   initTestCase
//---------------------------------------------------------

void TestAlbums::initTestCase()
{
    initMTest();
}

//---------------------------------------------------------
//   albumItemTest
//---------------------------------------------------------

void TestAlbums::albumAddScore()
{
    Album myAlbum;
    Album::activeAlbum = &myAlbum;
    MasterScore* aScore = readScore(DIR + "AlbumItemTest.mscx"); // deleted when aItem is deleted
    MasterScore* bScore = new MasterScore();
    AlbumItem* aItem = myAlbum.addScore(aScore, true); // deleted when myAlbum is deleted

    QCOMPARE(&aItem->album, &myAlbum);
    QVERIFY(myAlbum.albumItems().size() == 1);

    QVERIFY(Album::scoreInActiveAlbum(aScore));
    QVERIFY(!Album::scoreInActiveAlbum(bScore));

    QCOMPARE(aItem->setScore(bScore), -1);

    delete bScore;
}

//---------------------------------------------------------
//   albumItemDuration
//---------------------------------------------------------

void TestAlbums::albumItemDuration()
{
    Album myAlbum;
    Album::activeAlbum = &myAlbum;
    MasterScore* aScore = readScore(DIR + "AlbumItemTest.mscx");
    AlbumItem* aItem = myAlbum.addScore(aScore, true);

    QCOMPARE(aItem->duration(), aScore->duration());
    int scoreDuration1 = aScore->duration();
    aScore->appendMeasures(2);
    int scoreDuration2 = aScore->duration();
    QVERIFY(scoreDuration1 < scoreDuration2);
    QCOMPARE(aItem->duration(), aScore->duration());
}

//---------------------------------------------------------
//   albumItemEnable
//---------------------------------------------------------

void TestAlbums::albumItemEnable()
{
    Album myAlbum;
    Album::activeAlbum = &myAlbum;
    MasterScore* aScore = readScore(DIR + "AlbumItemTest.mscx");
    AlbumItem* aItem = myAlbum.addScore(aScore, true);

    QVERIFY(aItem->enabled());
    QCOMPARE(aItem->enabled(), aScore->enabled()); // true
    aItem->setEnabled(false);
    QVERIFY(!aItem->enabled());
    QCOMPARE(aItem->enabled(), aScore->enabled()); // false
}

//---------------------------------------------------------
//   albumItemBreaks
//---------------------------------------------------------

void TestAlbums::albumItemBreaks()
{
    Album myAlbum;
    Album::activeAlbum = &myAlbum;
    MasterScore* aScore = readScore(DIR + "AlbumItemTest.mscx");

    // addSectionBreak
    QCOMPARE(aScore->lastMeasure()->sectionBreak(), false);
    AlbumItem* aItem = myAlbum.addScore(aScore, true); // AlbumItem::addAlbumSectionBreak() gets called by Album::addScore
    QCOMPARE(aScore->lastMeasure()->sectionBreak(), true);

    // getSectionBreak
    QCOMPARE(aItem->getSectionBreak(), aScore->lastMeasure()->el().back());

    // m_pauseDuration part 1
    QCOMPARE(aItem->getSectionBreak()->pause(), aItem->m_pauseDuration);
    qreal newPauseDuration = aItem->m_pauseDuration * 2;
    aItem->getSectionBreak()->setPause(newPauseDuration);
    QCOMPARE(aItem->getSectionBreak()->pause(), newPauseDuration);

    // removeSectionBreak
    aItem->removeAlbumSectionBreak();
    QCOMPARE(aScore->lastMeasure()->sectionBreak(), false);

    // m_pauseDuration part 2
    aItem->addAlbumSectionBreak();
    QCOMPARE(aItem->getSectionBreak()->pause(), aItem->m_pauseDuration);

    // addPageBreak
    QCOMPARE(aScore->lastMeasure()->pageBreak(), false);
    aItem->addAlbumPageBreak();
    QCOMPARE(aScore->lastMeasure()->pageBreak(), true);

    // removePageBreak
    aItem->removeAlbumPageBreak();
    QCOMPARE(aScore->lastMeasure()->pageBreak(), false);
}

//---------------------------------------------------------
//   albumBreaks
//---------------------------------------------------------

void TestAlbums::albumBreaks()
{
    Album myAlbum;
    Album::activeAlbum = &myAlbum;
    MasterScore* aScore = readScore(DIR + "AlbumItemTest.mscx");
    MasterScore* bScore = readScore(DIR + "AlbumItemTest.mscx");
    myAlbum.addScore(aScore);
    myAlbum.addScore(bScore);
    myAlbum.addAlbumPageBreaks();
    QVERIFY(aScore->lastMeasure()->pageBreak());
    QVERIFY(bScore->lastMeasure()->pageBreak());
    QVERIFY(aScore->lastMeasure()->sectionBreak());
    QVERIFY(bScore->lastMeasure()->sectionBreak());
    myAlbum.removeAlbumPageBreaks();
    QVERIFY(!aScore->lastMeasure()->pageBreak());
    QVERIFY(!bScore->lastMeasure()->pageBreak());
    QVERIFY(aScore->lastMeasure()->sectionBreak());
    QVERIFY(bScore->lastMeasure()->sectionBreak());
    myAlbum.removeAlbumSectionBreaks();
    QVERIFY(!aScore->lastMeasure()->pageBreak());
    QVERIFY(!bScore->lastMeasure()->pageBreak());
    QVERIFY(!aScore->lastMeasure()->sectionBreak());
    QVERIFY(!bScore->lastMeasure()->sectionBreak());
}

//---------------------------------------------------------
//   realTitleNoCrash
///     regression test for a bugfix: Score::realTitle() used to index into
///     measure->el() unconditionally, which crashed (std::out_of_range /
///     null deref) for a score with no measures, or a first measure/frame
///     with no elements (i.e. no title text box).
//---------------------------------------------------------

void TestAlbums::realTitleNoCrash()
{
    MasterScore* emptyScore = new MasterScore();
    QVERIFY(emptyScore->realTitle().isEmpty());
    delete emptyScore;

    MasterScore* aScore = readScore(DIR + "AlbumItemTest.mscx");
    MeasureBase* firstMeasure = aScore->measures()->first();
    QVERIFY(firstMeasure);
    QVERIFY(!aScore->realTitle().isEmpty()); // sanity check: title frame is present before clearing

    firstMeasure->clearElements();
    QVERIFY(aScore->realTitle().isEmpty());

    delete aScore;
}

//---------------------------------------------------------
//   albumScoreTitlesNoCrash
///     regression test for the real-world trigger: creating an album and
///     adding two scores, where the second score's title frame has been
///     removed. Album::scoreTitles() calls Score::realTitle() for every
///     item, so this used to crash the whole application.
//---------------------------------------------------------

void TestAlbums::albumScoreTitlesNoCrash()
{
    Album myAlbum;
    Album::activeAlbum = &myAlbum;
    MasterScore* aScore = readScore(DIR + "AlbumItemTest.mscx");
    MasterScore* bScore = readScore(DIR + "AlbumItemTest.mscx");

    MeasureBase* firstMeasure = bScore->measures()->first();
    QVERIFY(firstMeasure);
    firstMeasure->clearElements(); // bScore now has no title text box

    myAlbum.addScore(aScore, true);
    myAlbum.addScore(bScore, true); // same parts as aScore, so no incompatible-parts prompt

    QStringList titles = myAlbum.scoreTitles(); // must not crash
    QCOMPARE(titles.size(), 2);
    QVERIFY(!titles.at(0).isEmpty());
    QCOMPARE(titles.at(1), bScore->title()); // realTitle() was empty, so falls back to score title
}

//---------------------------------------------------------
//   activeAlbumClearedOnDestruction
///     regression test: Album::activeAlbum used to stay set after the Album
///     it pointed to was destroyed. Score::doLayoutRange() unconditionally
///     dereferences Album::activeAlbum for every master-score layout, so a
///     dangling activeAlbum crashed on the very next score laid out
///     (found via ASAN: stack-buffer-overflow in Album::getCombinedScore()).
//---------------------------------------------------------

void TestAlbums::activeAlbumClearedOnDestruction()
{
    {
        Album myAlbum;
        Album::activeAlbum = &myAlbum;
        QVERIFY(Album::activeAlbum == &myAlbum);
    }
    // myAlbum is now destroyed; activeAlbum must not be left dangling
    QVERIFY(Album::activeAlbum == nullptr);

    // laying out an unrelated score must not dereference a stale activeAlbum
    MasterScore* aScore = readScore(DIR + "AlbumItemTest.mscx");
    delete aScore;
}

//---------------------------------------------------------
//   addScoreTwiceRejected
///     regression test: MuseScore::openScoreForAlbum() returns the existing
///     AlbumItem's MasterScore* when the "Add score" button is used again
///     on the same file already in the album. Album::addScore() used to
///     accept it anyway, wrapping the same MasterScore in a second
///     AlbumItem and adding it as a second movement to m_combinedScore,
///     which crashed layout with "ASSERT: curSystem != nextSystem".
//---------------------------------------------------------

void TestAlbums::addScoreTwiceRejected()
{
    Album myAlbum;
    Album::activeAlbum = &myAlbum;
    MasterScore* aScore = readScore(DIR + "AlbumItemTest.mscx");
    MasterScore* bScore = readScore(DIR + "AlbumItemTest.mscx");

    QVERIFY(myAlbum.addScore(aScore, true));
    QVERIFY(myAlbum.addScore(bScore, true));
    QCOMPARE(myAlbum.albumItems().size(), size_t(2));

    myAlbum.createCombinedScore(); // enters "Album mode", like the real addClicked() flow
    int movementCount = myAlbum.getCombinedScore()->movements()->size();

    // simulate clicking "Add score" again on a file already in the album:
    // openScoreForAlbum() would hand back aScore itself, not a fresh load
    AlbumItem* duplicate = myAlbum.addScore(aScore, true);
    QVERIFY(!duplicate);
    QCOMPARE(myAlbum.albumItems().size(), size_t(2));
    QCOMPARE(int(myAlbum.getCombinedScore()->movements()->size()), movementCount);

    // must not have corrupted the combined score's layout
    myAlbum.getCombinedScore()->doLayout();
}

QTEST_MAIN(TestAlbums)
#include "tst_albums.moc"

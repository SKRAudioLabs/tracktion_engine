/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2018
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com

    Tracktion Engine uses a GPL/commercial licence - see LICENCE.md for details.
*/

namespace tracktion { inline namespace engine
{

//==============================================================================
/**
    Holds a list of TempoSetting objects, to form a sequence of tempo changes.

    You can query this at particular points, but it's wise to use a
    TempoSequencePosition object to iterate it.
*/
class TempoSequence  : public Selectable,
                       private juce::AsyncUpdater
{
public:
    //==============================================================================
    TempoSequence (Edit&);
    ~TempoSequence() override;

    Edit& getEdit() const                           { return edit; }

    const juce::ValueTree& getState() const         { return state; }
    void setState (const juce::ValueTree&, bool remapEdit);
    void createEmptyState();

    void copyFrom (const TempoSequence&);
    void freeResources();

    //==============================================================================
    juce::UndoManager* getUndoManager() const noexcept;

    //==============================================================================
    const juce::Array<TimeSigSetting*>& getTimeSigs() const;
    int getNumTimeSigs() const;
    TimeSigSetting* getTimeSig (int index) const;
    TimeSigSetting& getTimeSigAt (TimePosition) const;
    TimeSigSetting& getTimeSigAtBeat (BeatPosition) const;

    int indexOfTimeSigAt (TimePosition time) const;
    int indexOfTimeSig (const TimeSigSetting*) const;

    //==============================================================================
    const juce::Array<TempoSetting*>& getTempos() const;
    int getNumTempos() const;
    TempoSetting* getTempo (int index) const;
    TempoSetting& getTempoAt (TimePosition) const;
    TempoSetting& getTempoAtBeat (BeatPosition) const;
    double getBpmAt (TimePosition) const; // takes ramping into account
    double getBeatsPerSecondAt (TimePosition, bool lengthOfOneBeatDependsOnTimeSignature = false) const;
    bool isTripletsAtTime (TimePosition) const;

    int indexOfTempoAt (TimePosition) const;
    int indexOfNextTempoAt (TimePosition) const;
    int indexOfTempo (const TempoSetting*) const;

    int countTemposInRegion (TimeRange) const;
    HashCode createHashForTemposInRange (TimeRange) const;

    /** inserts a tempo break that can be edited later. */
    TempoSetting::Ptr insertTempo (TimePosition);
    TempoSetting::Ptr insertTempo (BeatPosition, double bpm, float curve);
    TimeSigSetting::Ptr insertTimeSig (TimePosition);
    TimeSigSetting::Ptr insertTimeSig (BeatPosition);

    void removeTempo (int index, bool remapEdit);
    void removeTemposBetween (TimeRange, bool remapEdit);
    void removeTimeSig (int index);
    void removeTimeSigsBetween (TimeRange);

    void moveTempoStart (int index, BeatDuration deltaBeats, bool snapToBeat);
    void moveTimeSigStart (int index, BeatDuration deltaBeats, bool snapToBeat);

    /** Inserts space in to a sequence, shifting TempoSettings and TimeSigs. */
    void insertSpaceIntoSequence (TimePosition time, TimeDuration amountOfSpaceInSeconds, bool snapToBeat);

    /** Removes a region in a sequence, shifting TempoSettings and TimeSigs. */
    void deleteRegion (TimeRange);

    //==============================================================================
    /** Converts a time to a number of beats. */
    BeatPosition toBeats (TimePosition) const;

    /** Converts a time range to a beat range. */
    BeatRange toBeats (TimeRange) const;

    /** Converts a number of BarsAndBeats to a position. */
    BeatPosition toBeats (tempo::BarsAndBeats) const;

    /** Converts a number of beats a time. */
    TimePosition toTime (BeatPosition) const;

    /** Converts a beat range to a time range. */
    TimeRange toTime (BeatRange) const;

    /** Converts a number of BarsAndBeats to a position. */
    TimePosition toTime (tempo::BarsAndBeats) const;

    /** Converts a time to a number of BarsAndBeats. */
    tempo::BarsAndBeats toBarsAndBeats (TimePosition) const;

    //==============================================================================
    /** N.B. It is only safe to call this from the message thread or during audio callbacks.
        Access at any other time could incur data races.
    */
    const tempo::Sequence& getInternalSequence() const;

    //==============================================================================
    juce::String getSelectableDescription() override;

    //==============================================================================
    Edit& edit;

    //==============================================================================
    [[ deprecated ("Use new overload set above") ]] tempo::BarsAndBeats timeToBarsBeats (TimePosition) const;
    [[ deprecated ("Use new overload set above") ]] TimePosition barsBeatsToTime (tempo::BarsAndBeats) const;
    [[ deprecated ("Use new overload set above") ]] BeatPosition barsBeatsToBeats (tempo::BarsAndBeats) const;
    [[ deprecated ("Use new overload set above") ]] BeatPosition timeToBeats (TimePosition time) const;
    [[ deprecated ("Use new overload set above") ]] BeatRange timeToBeats (TimeRange range) const;
    [[ deprecated ("Use new overload set above") ]] TimePosition beatsToTime (BeatPosition beats) const;
    [[ deprecated ("Use new overload set above") ]] TimeRange beatsToTime (BeatRange range) const;

    /* @internal */
    void updateTempoData();

private:
    //==============================================================================
    friend class TempoSetting;
    friend class TimeSigSetting;

    //==============================================================================
    juce::ValueTree state;

    struct TempoSettingList;
    struct TimeSigList;
    std::unique_ptr<TempoSettingList> tempos;
    std::unique_ptr<TimeSigList> timeSigs;

    tempo::Sequence internalSequence { {{ BeatPosition(), 120.0, 0.0f }},
                                       {{ BeatPosition(), 4, 4, false }},
                                       tempo::LengthOfOneBeat::dependsOnTimeSignature };

    //==============================================================================
    void updateTempoDataIfNeeded() const;
    void handleAsyncUpdate() override;

    TempoSetting::Ptr insertTempo (BeatPosition, double bpm, float curve, juce::UndoManager*);
    TempoSetting::Ptr insertTempo (TimePosition, juce::UndoManager*);
    TimeSigSetting::Ptr insertTimeSig (TimePosition, juce::UndoManager*);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TempoSequence)
};

//==============================================================================
//==============================================================================
/** Creates a Position to iterate over the given TempoSequence.
    One of these lets you take a position in a tempo sequence, and skip forwards/backwards
    either by absolute times, or by bar/beat amounts.
    This is dynamic and if the TempoSequence changes, the position will update to reflect this.

    N.B. It is only safe to call this from the message thread or during audio callbacks.
    Access at any other time could incur data races.
*/
tempo::Sequence::Position createPosition (const TempoSequence&);


//==============================================================================
/** Takes a copy of all the beat related things in an edit in terms of bars/beats
    and then remaps these after a tempo change.
*/
struct EditTimecodeRemapperSnapshot
{
    void savePreChangeState (Edit&);
    void remapEdit (Edit&);

private:
    struct ClipPos
    {
        Selectable::WeakRef clip;
        BeatPosition startBeat, endBeat;
        BeatDuration contentStartBeat;
    };

    struct AutomationPos
    {
        AutomationCurve& curve;
        juce::Array<BeatPosition> beats;
    };

    juce::Array<ClipPos> clips;
    juce::Array<AutomationPos> automation;
    BeatRange loopPositionBeats;
};


}} // namespace tracktion { inline namespace engine

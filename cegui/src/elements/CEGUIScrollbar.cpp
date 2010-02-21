/***********************************************************************
    filename:   CEGUIScrollbar.cpp
    created:    13/4/2004
    author:     Paul D Turner
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2010 Paul D Turner & The CEGUI Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 ***************************************************************************/
#include "elements/CEGUIScrollbar.h"
#include "elements/CEGUIThumb.h"
#include "CEGUIWindowManager.h"
#include "CEGUIExceptions.h"

// Start of CEGUI namespace section
namespace CEGUI
{
//----------------------------------------------------------------------------//
const String Scrollbar::EventNamespace("Scrollbar");
const String Scrollbar::WidgetTypeName("CEGUI/Scrollbar");

//----------------------------------------------------------------------------//
ScrollbarProperties::DocumentSize   Scrollbar::d_documentSizeProperty;
ScrollbarProperties::PageSize       Scrollbar::d_pageSizeProperty;
ScrollbarProperties::StepSize       Scrollbar::d_stepSizeProperty;
ScrollbarProperties::OverlapSize    Scrollbar::d_overlapSizeProperty;
ScrollbarProperties::ScrollPosition Scrollbar::d_scrollPositionProperty;

//----------------------------------------------------------------------------//
const String Scrollbar::EventScrollPositionChanged("ScrollPosChanged");
const String Scrollbar::EventThumbTrackStarted("ThumbTrackStarted");
const String Scrollbar::EventThumbTrackEnded("ThumbTrackEnded");
const String Scrollbar::EventScrollConfigChanged("ScrollConfigChanged");

//----------------------------------------------------------------------------//
const String Scrollbar::ThumbNameSuffix("__auto_thumb__");
const String Scrollbar::IncreaseButtonNameSuffix("__auto_incbtn__");
const String Scrollbar::DecreaseButtonNameSuffix("__auto_decbtn__");

//----------------------------------------------------------------------------//
ScrollbarWindowRenderer::ScrollbarWindowRenderer(const String& name) :
        WindowRenderer(name, Scrollbar::EventNamespace)
{
}

//----------------------------------------------------------------------------//
Scrollbar::Scrollbar(const String& type, const String& name) :
    Window(type, name),
    d_documentSize(1.0f),
    d_pageSize(0.0f),
    d_stepSize(1.0f),
    d_overlapSize(0.0f),
    d_position(0.0f)
{
    addScrollbarProperties();
}

//----------------------------------------------------------------------------//
Scrollbar::~Scrollbar(void)
{
}

//----------------------------------------------------------------------------//
void Scrollbar::initialiseComponents(void)
{
    // Set up thumb
    Thumb* const t = getThumb();
    t->subscribeEvent(Thumb::EventThumbPositionChanged,
                      Event::Subscriber(&CEGUI::Scrollbar::handleThumbMoved,
                      this));

    t->subscribeEvent(Thumb::EventThumbTrackStarted,
                      Event::Subscriber(&CEGUI::Scrollbar::handleThumbTrackStarted,
                      this));

    t->subscribeEvent(Thumb::EventThumbTrackEnded,
                      Event::Subscriber(&CEGUI::Scrollbar::handleThumbTrackEnded,
                      this));

    // set up Increase button
    getIncreaseButton()->
        subscribeEvent(PushButton::EventMouseButtonDown,
                       Event::Subscriber(&CEGUI::Scrollbar::handleIncreaseClicked,
                       this));

    // set up Decrease button
    getDecreaseButton()->
        subscribeEvent(PushButton::EventMouseButtonDown,
                       Event::Subscriber(&CEGUI::Scrollbar::handleDecreaseClicked,
                       this));

    // do initial layout
    performChildWindowLayout();
}

//----------------------------------------------------------------------------//
void Scrollbar::setDocumentSize(float document_size)
{
    if (d_documentSize != document_size)
    {
        d_documentSize = document_size;
        updateThumb();

        WindowEventArgs args(this);
        onScrollConfigChanged(args);
    }
}

//----------------------------------------------------------------------------//
void Scrollbar::setPageSize(float page_size)
{
    if (d_pageSize != page_size)
    {
        d_pageSize = page_size;
        updateThumb();

        WindowEventArgs args(this);
        onScrollConfigChanged(args);
    }
}

//----------------------------------------------------------------------------//
void Scrollbar::setStepSize(float step_size)
{
    if (d_stepSize != step_size)
    {
        d_stepSize = step_size;

        WindowEventArgs args(this);
        onScrollConfigChanged(args);
    }
}

//----------------------------------------------------------------------------//
void Scrollbar::setOverlapSize(float overlap_size)
{
    if (d_overlapSize != overlap_size)
    {
        d_overlapSize = overlap_size;

        WindowEventArgs args(this);
        onScrollConfigChanged(args);
    }
}

//----------------------------------------------------------------------------//
void Scrollbar::setScrollPosition(float position)
{
    const float old_pos = d_position;

    // max position is (docSize - pageSize)
    // but must be at least 0 (in case doc size is very small)
    const float max_pos = ceguimax((d_documentSize - d_pageSize), 0.0f);

    // limit position to valid range:  0 <= position <= max_pos
    d_position = (position >= 0) ?
                    ((position <= max_pos) ?
                        position :
                        max_pos) :
                    0.0f;

    updateThumb();

    // notification if required
    if (d_position != old_pos)
    {
        WindowEventArgs args(this);
        onScrollPositionChanged(args);
    }
}

//----------------------------------------------------------------------------//
void Scrollbar::onScrollPositionChanged(WindowEventArgs& e)
{
    fireEvent(EventScrollPositionChanged, e, EventNamespace);
}

//----------------------------------------------------------------------------//
void Scrollbar::onThumbTrackStarted(WindowEventArgs& e)
{
    fireEvent(EventThumbTrackStarted, e, EventNamespace);
}

//----------------------------------------------------------------------------//
void Scrollbar::onThumbTrackEnded(WindowEventArgs& e)
{
    fireEvent(EventThumbTrackEnded, e, EventNamespace);
}

//----------------------------------------------------------------------------//
void Scrollbar::onScrollConfigChanged(WindowEventArgs& e)
{
    performChildWindowLayout();
    fireEvent(EventScrollConfigChanged, e, EventNamespace);
}

//----------------------------------------------------------------------------//
void Scrollbar::onMouseButtonDown(MouseEventArgs& e)
{
    // base class processing
    Window::onMouseButtonDown(e);

    if (e.button == LeftButton)
    {
        const float adj = getAdjustDirectionFromPoint(e.position);

        // adjust scroll bar position in whichever direction as required.
        if (adj != 0)
            setScrollPosition(
                d_position + ((d_pageSize - d_overlapSize) * adj));

        ++e.handled;
    }
}

//----------------------------------------------------------------------------//
void Scrollbar::onMouseWheel(MouseEventArgs& e)
{
    // base class processing
    Window::onMouseWheel(e);

    // scroll by e.wheelChange * stepSize
    setScrollPosition(d_position + d_stepSize * -e.wheelChange);

    // ensure the message does not go to our parent.
    ++e.handled;
}

//----------------------------------------------------------------------------//
bool Scrollbar::handleThumbMoved(const EventArgs&)
{
    // adjust scroll bar position as required.
    setScrollPosition(getValueFromThumb());

    return true;
}

//----------------------------------------------------------------------------//
bool Scrollbar::handleIncreaseClicked(const EventArgs& e)
{
    if (((const MouseEventArgs&)e).button == LeftButton)
    {
        // adjust scroll bar position as required.
        setScrollPosition(d_position + d_stepSize);

        return true;
    }

    return false;
}

//----------------------------------------------------------------------------//
bool Scrollbar::handleDecreaseClicked(const EventArgs& e)
{
    if (((const MouseEventArgs&)e).button == LeftButton)
    {
        // adjust scroll bar position as required.
        setScrollPosition(d_position - d_stepSize);

        return true;
    }

    return false;
}

//----------------------------------------------------------------------------//
bool Scrollbar::handleThumbTrackStarted(const EventArgs&)
{
    // simply trigger our own version of this event
    WindowEventArgs args(this);
    onThumbTrackStarted(args);

    return true;
}

//----------------------------------------------------------------------------//
bool Scrollbar::handleThumbTrackEnded(const EventArgs&)
{
    // simply trigger our own version of this event
    WindowEventArgs args(this);
    onThumbTrackEnded(args);

    return true;
}

//----------------------------------------------------------------------------//
void Scrollbar::addScrollbarProperties(void)
{
    addProperty(&d_documentSizeProperty);
    addProperty(&d_pageSizeProperty);
    addProperty(&d_stepSizeProperty);
    addProperty(&d_overlapSizeProperty);
    addProperty(&d_scrollPositionProperty);

    // we ban all these properties from xml for auto windows
    if (isAutoWindow())
    {
        banPropertyFromXML(&d_documentSizeProperty);
        banPropertyFromXML(&d_pageSizeProperty);
        banPropertyFromXML(&d_stepSizeProperty);
        banPropertyFromXML(&d_overlapSizeProperty);
        banPropertyFromXML(&d_scrollPositionProperty);

        // scrollbars tend to have their visibility toggled alot, so we ban
        // that as well
        banPropertyFromXML(&d_visibleProperty);
    }
}

//----------------------------------------------------------------------------//
PushButton* Scrollbar::getIncreaseButton() const
{
    return static_cast<PushButton*>(WindowManager::getSingleton().getWindow(
                                        getName() + IncreaseButtonNameSuffix));
}

//----------------------------------------------------------------------------//
PushButton* Scrollbar::getDecreaseButton() const
{
    return static_cast<PushButton*>(WindowManager::getSingleton().getWindow(
                                        getName() + DecreaseButtonNameSuffix));
}

//----------------------------------------------------------------------------//
Thumb* Scrollbar::getThumb() const
{
    return static_cast<Thumb*>(WindowManager::getSingleton().getWindow(
                                   getName() + ThumbNameSuffix));
}

//----------------------------------------------------------------------------//
void Scrollbar::updateThumb(void)
{
    if (!d_windowRenderer)
        throw InvalidRequestException("Scrollbar::updateThumb: This function "
            "must be implemented by the window renderer object (no window "
            "renderer is assigned.)");

    static_cast<ScrollbarWindowRenderer*>(d_windowRenderer)->updateThumb();
}

//----------------------------------------------------------------------------//
float Scrollbar::getValueFromThumb(void) const
{
    if (!d_windowRenderer)
        throw InvalidRequestException("Scrollbar::getValueFromThumb: This "
            "function must be implemented by the window renderer object (no "
            "window renderer is assigned.)");

    return static_cast<ScrollbarWindowRenderer*>(
        d_windowRenderer)->getValueFromThumb();
}

//----------------------------------------------------------------------------//
float Scrollbar::getAdjustDirectionFromPoint(const Point& pt) const
{
    if (!d_windowRenderer)
        throw InvalidRequestException("Scrollbar::getAdjustDirectionFromPoint: "
            "This function must be implemented by the window renderer object "
            "(no window renderer is assigned.)");

    return static_cast<ScrollbarWindowRenderer*>(
        d_windowRenderer)->getAdjustDirectionFromPoint(pt);
}

//----------------------------------------------------------------------------//

} // End of  CEGUI namespace section

